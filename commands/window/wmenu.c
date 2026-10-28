/* Resident window manager. It stays a Concurrent process, detaches from its
 * console, and sleeps on XIOS ww_key until the window-manager hotkey wakes it.
 * No DOS TSR or CP/M retained-memory termination is involved.
 */
#include "win.h"
extern int dataseg();
static char status[160];
static line(s) char *s;
{
    int i;
    for(i=0;i<80;++i) { status[2*i]=*s?*s++:' '; status[2*i+1]=0x70; }
    xcall(18,0,dataseg(),status);
}
static restore_line() { xcall(8,0,0xffff,0); }
static letter(k) int k;
{
    k&=255;
    return k>='a' && k<='z'?k-32:k;
}
static edit(n,mode) int n,mode;
{
    struct window w,saved; int k,r,c;
    w_read(n,&saved); w=saved;
    line("Arrow keys change window; RETURN accepts, ESC restores previous settings");
    for(;;) {
        k=w_key();
        if(k==13) return;
        if(k==27) { w_apply(&saved); return; }
        r=c=0;
        if(k==0xff0b) r=-1;
        if(k==0xff10) r=1;
        if(k==0xff0d) c=-1;
        if(k==0xff0e) c=1;
        if(!r && !c) continue;
        if(mode==0) { w.prow+=r; w.pcol+=c; }
        if(mode==1) { w.nrows+=r; w.ncols+=c; }
        if(mode==2) { w.vrow+=r; w.vcol+=c; }
        if(!w_apply(&w)) w_read(n,&w);
    }
}
static color(n) int n;
{
    struct window w,saved; int k,background;
    w_read(n,&saved); w=saved; background=0;
    line("Color: 0 Black 1 Blue 2 Green 3 Cyan 4 Red 5 Magenta 6 Yellow 7 White; TAB F/B");
    for(;;) {
        k=w_key();
        if(k==13) return;
        if(k==27) { w_apply(&saved); return; }
        if(k==9) background=!background;
        if(k>='0' && k<='7') {
            if(background) w.background=k-'0'; else w.foreground=k-'0';
            w_apply(&w);
        }
    }
}
static write_file(n) int n;
{
    char name[20],prompt[160]; int used,k,type;
    used=0; type=0;
    for(;;) {
        name[used]=0;
        format(prompt,"Write %s: %s  TAB changes type; RETURN writes; ESC cancels",
          type==0?"window":type==1?"console":"setup",name);
        line(prompt); k=w_key();
        if(k==27) return;
        if(k==9) { type=(type+1)%3; continue; }
        if(k==8 && used) { --used; continue; }
        if(k==13 && used) {
            if(w_write(n,type,name)) return;
            line("Error writing file, check disk. Press a key."); w_key(); return;
        }
        if(k>=32 && k<=126 && used<19) name[used++]=letter(k);
    }
}
main()
{
    struct window w; int n,k,active,done;
    char prompt[160];
    if(!w_init()) {
        message("WMENU requires Concurrent CP/M system that supports window management.\r\n");
        return;
    }
    if(w_status(0)) { message("Window Manager already installed.\r\n"); return; }
    w_manager(1,255);
    message("Window Manager installed.\r\n");
    bdos(147,0); /* C_DETACH: return the launching console to its shell. */
    done=0;
    while(!done) {
        w_key(); /* Idle state: XIOS hotkey activation wakes this process. */
        n=w_status(2); active=1; w_manager(2,255);
        while(active && !done) {
            format(prompt,"N=%d: P Place S Size R Scroll T Tracking D Display C Color W Write A Abort ESC Exit",n);
            line(prompt); k=letter(w_key());
            if(k>='0' && k<'0'+w_status(1)) { n=k-'0'; w_top(n); }
            else if(k==27) active=0;
            else if(k=='P') edit(n,0);
            else if(k=='S') edit(n,1);
            else if(k=='R') edit(n,2);
            else if(k=='C') color(n);
            else if(k=='W') write_file(n);
            else if(k=='T' || k=='D') {
                w_read(n,&w);
                if(k=='T') w.tracking=!w.tracking; else w.display=!w.display;
                w_apply(&w);
            } else if(k=='A') {
                line("Press RETURN to abort WMENU, ESC to return to the menu");
                k=w_key(); if(k==13) done=1;
            }
        }
        restore_line(); w_manager(done?0:1,255);
    }
}
