/* CDOS WMENU: the native highlighted menu and live XIOS editing model.
 * Public window numbers are 1..4; the service layer always uses 0..3.
 * ESC/RETURN leave an editing submenu with its changes in effect.
 */
#include "win.h"
extern int dataseg();
static char status[160];
static int starts[]={6,12,17,24,33,41,47,53};
static int lengths[]={5,4,6,8,7,5,5,5};
static setline(s) char *s;
{
    int i;
    for(i=0;i<80;++i) { status[2*i]=*s?*s++:' '; status[2*i+1]=7; }
}
static mark(p,n) int p,n;
{ while(n-- && p<80) { status[2*p+1]=0x70; ++p; } }
static showline() { xcall(18,0,status,dataseg()); }
static line(s) char *s; { setline(s); showline(); }
static fields(s,a,b,c,d) char *s; int a,b,c,d;
{ setline(s); mark(a,b); mark(c,d); showline(); }
static restore_line() { xcall(8,0,0xffff,0); }
static previous(k) int k; { return k==0xff0b || k==0xff0d; }
static next(k) int k; { return k==0xff10 || k==0xff0e || k==' '; }
static edit(n,mode) int n,mode;
{
    struct window w; int k,r,c;
    char prompt[160];
    w_read(n,&w);
    for(;;) {
        if(mode==0) format(prompt,"WINDOW CHANGE N=%d PRow=%02d,PCol=%02d             (Pressing \030,\031,\032,\033 places window)",n+window_first,w.prow,w.pcol);
        if(mode==1) format(prompt,"WINDOW CHANGE N=%d NRows=%02d,NCols=%02d            (Pressing \030,\031,\032,\033 sizes window)",n+window_first,w.nrows,w.ncols);
        if(mode==2) format(prompt,"WINDOW CHANGE N=%d VRow=%02d,VCol=%02d             (Pressing \030,\031,\032,\033 scrolls window)",n+window_first,w.vrow,w.vcol);
        fields(prompt,mode==1?24:23,2,mode==1?33:31,2);
        k=w_key(); if(k==13 || k==27) return;
        r=c=0;
        if(k==0xff0b) r=-1; if(k==0xff10) r=1;
        if(k==0xff0d) c=-1; if(k==0xff0e) c=1;
        if(!r && !c) continue;
        if(mode==0) { w.prow+=r; w.pcol+=c; }
        if(mode==1) { w.nrows+=r; w.ncols+=c; }
        if(mode==2) { w.vrow+=r; w.vcol+=c; }
        w_apply(&w); w_read(n,&w);
    }
}
static toggle(n,display) int n,display;
{
    struct window w; int k; char prompt[160];
    w_read(n,&w);
    for(;;) {
        if(display) format(prompt,"WINDOW CHANGE N=%d Display=%s    (Pressing \030,\031 changes display style)",n+window_first,w.display?"Color":"B&W  ");
        else format(prompt,"WINDOW CHANGE N=%d Tracking=%s    (Pressing \030,\031 changes cursor tracking style)",n+window_first,w.tracking?"Row":"No ");
        fields(prompt,display?26:27,display?5:3,0,0);
        k=w_key(); if(k==13 || k==27) return;
        if(k==' ' || k==0xff0b || k==0xff10) {
            if(display) w.display=!w.display; else w.tracking=!w.tracking;
            w_apply(&w); w_read(n,&w);
        }
    }
}
static color_name(dst,n) char *dst; int n;
{ int i; for(i=0;i<7;++i) dst[i]=' '; dst[7]=0;
  for(i=0;colors[n][i];++i) dst[i]=colors[n][i]; }
static color(n) int n;
{
    struct window w; int k,background,delta; char prompt[160],fg[8],bg[8];
    w_read(n,&w); background=0;
    for(;;) {
        color_name(fg,w.foreground); color_name(bg,w.background);
        format(prompt,"WINDOW CHANGE N=%d FG=%s,BG=%s     (Pressing \030,\031,\032,\033 changes colors)",n+window_first,fg,bg);
        fields(prompt,background?32:21,7,0,0);
        k=w_key(); if(k==13 || k==27) return;
        if(k==9 || k==0xff0d || k==0xff0e) { background=!background; continue; }
        delta=(k==' ' || k==0xff10)?1:k==0xff0b?-1:0;
        if(delta) {
            if(background) w.background=(w.background+delta+8)%8;
            else w.foreground=(w.foreground+delta+8)%8;
            w_apply(&w); w_read(n,&w);
        }
    }
}
static write_file(n) int n;
{
    char name[20],prompt[160]; int used,k,type,typing;
    type=typing=0; name[0]='A'+bdos(25,0); name[1]=':';
    format(name+2,"WINDOW%d.TXT",n+window_first);
    for(used=0;name[used];++used) ;
    for(;;) {
        name[used]=0;
        format(prompt,"WINDOW WRITE N=%d Type=%s,File=%s         (Pressing \030,\031 changes type)",n+window_first,type==0?"W":type==1?"C":"S",name);
        fields(prompt,22,1,29,used);
        k=w_key(); if(k==27) return;
        if(k==9 || k==' ' || k==0xff0b) { type=(type+1)%3; continue; }
        if(k==0xff10) { type=(type+2)%3; continue; }
        if(k==24) { used=0; typing=1; continue; }
        if((k==8 || k==127) && used) { --used; typing=1; continue; }
        if(k==13 && used) {
            if(w_write(n,type,name)) return;
            line("Error writing file, check disk. Press a key."); w_key(); return;
        }
        if(k>=33 && k<=126) {
            if(!typing) { used=0; typing=1; }
            if(used<19) name[used++]=(k>='a' && k<='z')?k-32:k;
        }
    }
}
main()
{
    int n,k,active,done,choice,run; char prompt[160];
    if(!w_init()) { message("WMENU requires Concurrent CP/M system that supports window management.\r\n"); return; }
    if(w_status(0)) { message("Window Manager already installed.\r\n"); return; }
    w_manager(1,255); message("Window Manager installed.\r\n");
    bdos(147,0); /* Detach; the XIOS manager hotkey wakes the sleeping process. */
    done=choice=0;
    while(!done) {
        w_key(); n=w_status(2); active=1; w_manager(2,255);
        while(active && !done) {
            format(prompt,"N=%d:  Place Size Scroll Tracking Display Color Write Abort         [ESC]->Exit",n+window_first);
            fields(prompt,starts[choice],lengths[choice],0,0);
            k=w_key(); run=0;
            if(k==27) active=0;
            else if(previous(k)) choice=(choice+7)%8;
            else if(next(k)) choice=(choice+1)%8;
            else if(k==13) run=1;
            else if(k>='0'+window_first && k<'0'+window_first+w_status(1)) {
                n=k-'0'-window_first; w_top(n);
            }
            if(!run) continue;
            if(choice<3) edit(n,choice);
            else if(choice<5) toggle(n,choice==4);
            else if(choice==5) color(n);
            else if(choice==6) write_file(n);
            else {
                format(prompt,"N=%d:        (Press [RETURN] to abort WMENU, [ESC] to go back to previous menu)",n+window_first);
                line(prompt); k=w_key(); if(k==13) done=1;
            }
        }
        restore_line(); w_manager(done?0:1,255);
    }
}
