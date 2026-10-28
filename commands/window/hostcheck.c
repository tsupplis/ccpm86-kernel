/* Behavioral checks for the portable C logic; this simulates the documented
 * XIOS ABI, not a second implementation of the original command parser. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "win.h"
static unsigned char mem[8192];
static int calls;
static int version=0x1431;
int XINIT() { XDATA=1; return 0; }
int FARWORD(seg,p) int seg,p; { return mem[p]|(mem[p+1]<<8); }
int FARBYTE(seg,p) int seg,p; { return mem[p]; }
int FARPUT(seg,p,v) int seg,p,v; { mem[p]=v; mem[p+1]=v>>8; return 0; }
int BDOS(fn,arg) int fn,arg; { return fn==12?version:0; }
int XCALL(fn,b,c,d) int fn,b,c,d;
{
    int p=1024+128*(d&255);
    ++calls;
    if(fn==16) return d==255?512:p;
    if(fn==20) { FARPUT(1,p+2,c); FARPUT(1,p+4,b); }
    if(fn==21) { FARPUT(1,p+14,c); mem[p+30]=d>>8; }
    if(fn==24) FARPUT(1,p+24,c?0xb800:0xb000);
    if(fn==19 && c<3) mem[512]=c;
    return 0;
}
static int command(s) char *s;
{
    char t[256]; strcpy(t,s); return w_command(t);
}
int main()
{
    struct window w,before;
    int i,old;
    mem[513]=4;
    for(i=0;i<4;++i) {
        FARPUT(1,1024+128*i+4,0x174f);
        FARPUT(1,1024+128*i+24,0xb800);
        mem[1024+128*i+28]=7;
    }
    version=0x22; assert(!w_init());
    version=0x1430; assert(!w_init());
    version=0x31; assert(!w_init());
    version=0x1431; assert(w_init());
    assert(w_read(1,&w) && w.nrows==24 && w.ncols==80);
    assert(command("change n=1 pr=4,pc=5,nr=10,nc=20,vr=2,vc=3,tr=row,d=b&w,fg=red,bg=blue"));
    assert(w_read(1,&w));
    assert(w.prow==4 && w.pcol==5 && w.nrows==10 && w.ncols==20);
    assert(w.vrow==2 && w.vcol==3 && w.tracking==1 && w.display==0);
    assert(w.foreground==4 && w.background==1);
    before=w; old=calls;
    assert(!command("change n=1 pr=25"));
    assert(w_read(1,&w) && !memcmp(&before,&w,sizeof(w)));
    assert(!command("change n=1 unknown=4"));
    assert(!command("change n=1 pr=4junk"));
    assert(!command("top n=4"));
    assert(!command("top"));
    assert(!command("write n=0 file=TEST.TXT"));
    assert(!command("write n=0 type=setup"));
    assert(command("change n=1 prow=9,pcol=62,nrows=24,ncols=80"));
    assert(w_read(1,&w) && w.nrows==16 && w.ncols==19);
    assert(command("top n=0"));
    assert(command("full n=1"));
    puts("PASS: parser, ranges, state conversion and XIOS dispatch checks");
    return 0;
}
