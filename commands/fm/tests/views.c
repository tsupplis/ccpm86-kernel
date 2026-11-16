/* Isolated view tests: links only model.c and view.c, with injected output.
 * No fixture adapter, controller, native library, global replacement or PCE.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fm.h"

struct recording {
    unsigned char cells[24][80];
    unsigned char attrs[24][80];
    int writes, begins, ends, fail_write, fail_end;
};
static int begin(void *context)
{
    struct recording *r = context;
    ++r->begins;
    return FM_SUCCESS;
}
static int write_run(void *context, int x, int y, int attr, const char *text, int length)
{
    struct recording *r = context;
    int i;
    assert(x >= 0 && y >= 0 && y < 24 && length >= 0 && x + length <= 80);
    if (++r->writes == r->fail_write) return FM_OUTPUT_ERROR;
    for (i = 0; i < length; ++i) {
        r->cells[y][x+i] = (unsigned char)text[i];
        r->attrs[y][x+i] = (unsigned char)attr;
    }
    return FM_SUCCESS;
}
static int end(void *context)
{
    struct recording *r = context;
    ++r->ends;
    return r->fail_end ? FM_OUTPUT_ERROR : FM_SUCCESS;
}
static struct fm_panel panel(int x, int y, int w, int h)
{
    struct fm_panel p = {{0,0,0,0},{7,112,14,15,7}};
    p.rect.x=x; p.rect.y=y; p.rect.width=w; p.rect.height=h;
    return p;
}
int main(void)
{
    struct recording r = {{{0}},{{0}},0,0,0,0,0};
    struct fm_display d = {&r,begin,write_run,end};
    struct fm_model m = {0}, before;
    const struct fm_file files[] = {{"ONE", "1 byte", 0},{"TWO", "2 bytes", 0},{"THREE", "3 bytes", 0},{"FOUR", "4 bytes", 0},{"FIVE", "5 bytes", 0}};
    const struct fm_command commands[] = {{21,"Alpha","Alpha help",FM_TARGET_NONE},{37,"Beta","Beta help",FM_TARGET_NONE},{40,"Gamma","Gamma help",FM_TARGET_NONE}};
    const struct fm_hint hint[] = {{0,1,"Return"}};
    int writes;

    m.device.panel=panel(0,0,30,4); m.device.media_width=8;
    m.device.media="Media"; m.device.capacity="10k"; m.device.location="Path";
    m.files.panel=panel(0,3,30,6); m.files.items=files; m.files.count=5;
    m.files.columns=2; m.files.cursor.selected=1; m.files.focused=1;
    m.files.heading="Files"; m.files.empty_text="Empty"; fm_mark_set(&m.files, 1, 1);
    m.actions.columns=1; m.actions.panel=panel(30,0,20,4); m.actions.items=commands; m.actions.count=3;
    m.actions.cursor.selected=0; m.actions.empty_text="None";
    m.help.panel=panel(0,21,80,3); m.help.status="Status"; m.help.hints=hint; m.help.count=1;
    /* Publication certifies immutable names, never cursor/layout bounds.
     * Replacement or changed length must fall back to full validation. */
    {
        struct fm_file bad[5];
        memcpy(bad, files, sizeof(bad));
        assert(!fm_files_publish(&m.files));
        assert(m.files.validated_items == files && m.files.validated_count == 5);
        m.files.cursor.selected = 5;
        assert(!fm_files_valid(&m.files));
        m.files.cursor.selected = 1;
        bad[4].name = 0;
        m.files.items = bad;
        assert(!fm_files_valid(&m.files));
        assert(fm_files_publish(&m.files) == FM_BAD_MODEL);
        m.files.items = files;
        assert(!fm_files_publish(&m.files));
        m.files.validated_count = -1; /* Owner revokes before content changes. */
        assert(fm_files_valid(&m.files));
    }
    /* Bit boundaries, the final catalog slot and rejected indices must not
     * affect neighbouring marks or write outside the compact storage. */
    {
        struct fm_files_model marks = {0};
        marks.count = FM_MAX_FILES;
        assert(!fm_mark_set(&marks, 7, 1));
        assert(!fm_mark_set(&marks, 8, 1));
        assert(!fm_mark_set(&marks, 1023, 1));
        assert(fm_marked(&marks, 7) && fm_marked(&marks, 8));
        assert(fm_marked(&marks, 1023) && !fm_marked(&marks, 1022));
        assert(!fm_mark_set(&marks, 7, 0));
        assert(!fm_marked(&marks, 7) && fm_marked(&marks, 8));
        assert(fm_mark_set(&marks, -1, 1) == FM_BAD_MODEL);
        assert(fm_mark_set(&marks, 1024, 1) == FM_BAD_MODEL);
        assert(!fm_marked(&marks, -1) && !fm_marked(&marks, 1024));
        marks.count = 8;
        assert(!fm_marked(&marks, 8));
        assert(fm_mark_set(&marks, 8, 0) == FM_BAD_MODEL);
        fm_marks_clear(&marks);
        marks.count = FM_MAX_FILES;
        assert(!fm_marked(&marks, 8) && !fm_marked(&marks, 1023));
    }
    before=m;
    assert(fm_render(&m,&d)==FM_SUCCESS);
    assert(!memcmp(&m,&before,sizeof(m)));
    assert(r.cells[3][0]==195 && r.cells[3][29]==180 && r.cells[3][9]==193);
    assert(r.cells[7][29]==31); /* More files below. */
    assert(r.cells[2][49]==31); /* More actions below. */
    assert(r.attrs[5][25]==112); /* Whole selected field, including spaces. */
    assert(r.begins==1 && r.ends==1);
    assert(r.cells[5][15] == '*' && r.cells[5][16] == 'T');
    assert(r.attrs[5][15] == m.files.panel.style.normal);
    assert(r.attrs[5][16] == m.files.panel.style.selected);
    assert(r.cells[1][31] == ' ' && r.cells[1][32] == 'A');

    /* Standalone renderers use only their own model and the output contract. */
    assert(!fm_device_render(&m.device,&d));
    assert(!fm_files_render(&m.files,&d));
    assert(!fm_actions_render(&m.actions,&d));
    assert(!fm_help_render(&m.help,&d));
    m.files.focused=0; m.actions.focused=1;
    assert(!fm_files_render(&m.files,&d));
    assert(!fm_actions_render(&m.actions,&d));
    assert(r.cells[5][15] == '*' && r.cells[1][31] == 16);
    assert(r.attrs[1][31] == m.actions.panel.style.normal);
    assert(r.attrs[1][32] == m.actions.panel.style.selected);
    assert(r.attrs[5][25]==14); /* Explicit target, not keyboard focus. */

    /* No marks: current file is the implicit target, without a star/caret
     * while Actions owns focus. An off-screen mark suppresses that target. */
    fm_mark_set(&m.files, 1, 0);
    assert(!fm_files_render(&m.files, &d));
    assert(r.attrs[5][16] == m.files.panel.style.marked);
    assert(r.cells[5][15] == ' ');
    fm_mark_set(&m.files, 4, 1);
    assert(!fm_files_render(&m.files, &d));
    assert(r.attrs[5][16] == m.files.panel.style.normal);
    assert(r.cells[5][15] == ' ');
    m.files.focused = 1;
    m.actions.focused = 0;
    assert(!fm_files_render(&m.files, &d));
    assert(!fm_actions_render(&m.actions, &d));
    assert(r.attrs[5][16] == m.files.panel.style.selected);
    assert(r.cells[5][15] == 16 && r.cells[1][31] == ' ');
    fm_mark_set(&m.files, 4, 0);
    assert(!fm_files_render(&m.files, &d));
    assert(r.attrs[5][16] == m.files.panel.style.selected);

    m.files.focused = 0;
    m.actions.focused = 1;
    m.dialog.panel=panel(8,7,40,8); m.dialog.kind=FM_DIALOG_INPUT;
    m.dialog.title="Input"; strcpy(m.dialog.text,"Edit");
    memset(m.dialog.input,'x',60); m.dialog.input[60]=0; m.dialog.position=60;
    before=m;
    assert(!fm_render(&m,&d));
    assert(!memcmp(&m,&before,sizeof(m)));
    assert(r.attrs[11][45]==112 && r.cells[11][45]==' ');
    m.dialog.position=61;
    writes=r.writes;
    assert(fm_render(&m,&d)==FM_BAD_MODEL && r.writes==writes);
    m.dialog.position=60; memset(m.dialog.text,'x',80);
    assert(fm_render(&m,&d)==FM_BAD_MODEL && r.writes==writes);
    m.dialog.text[0]=0; m.dialog.kind=FM_DIALOG_PICK;
    m.dialog.count=1; m.dialog.first=0; m.dialog.selected=0; m.dialog.options[0]=0;
    assert(fm_render(&m,&d)==FM_BAD_MODEL && r.writes==writes);
    m.dialog.options[0]="An option";
    assert(!fm_render(&m,&d));
    m.files.focused=1;
    assert(fm_render(&m,&d)==FM_BAD_MODEL); /* Cannot focus both panes. */
    m.files.focused=0;
    m.dialog.kind=FM_DIALOG_NONE;
    r.fail_write=r.writes+1;
    writes=r.ends;
    assert(fm_render(&m,&d)==FM_OUTPUT_ERROR && r.ends==writes+1);
    r.fail_write=0; r.fail_end=1;
    assert(fm_render(&m,&d)==FM_OUTPUT_ERROR);
    puts("PASS: isolated views, clipping, style precedence, junctions, scroll indicators, long input, modal prevalidation, focus and output failures");
    return 0;
}
