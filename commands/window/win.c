#define WIN_IMPLEMENTATION
/* Shared window operations. This is a source rewrite, not a byte-identical
 * disassembly. Keep the XIOS ABI and one-based public coordinates explicit.
 */
#include "win.h"
WORD xdata=0;
char *colors[] = {"Black","Blue","Green","Cyan","Red","Magenta","Yellow","White"};

conchar(c) int c; { bdos(2,c); return 0; }
message(s) char *s; { while (*s) conchar(*s++); return 0; }

/* BDOS 12 identifies the operating system and its BDOS revision. The
 * Concurrent 8086 identity is 1400h and the required revision is 31h.
 * Check both before using the private XIOS gateway. */
w_init()
{
    int version;
    version=bdos(12,0);
    if ((version&0xff00)!=0x1400 || (version&255)<0x31) return 0;
    xinit();
    return 1;
}

w_status(which) int which;
{
    int p;
    p=xcall(16,0,0,255);
    if (which==0) return farbyte(xdata,p);
    if (which==1) return farbyte(xdata,p+1);
    return farbyte(xdata,p+1+farbyte(xdata,p+1));
}

w_read(n,w) int n; struct window *w;
{
    int p,tl,br,v,attr;
    if (n<0 || n>=w_status(1)) return 0;
    p=xcall(16,0,0,n);
    tl=farword(xdata,p+2); br=farword(xdata,p+4);
    v=farword(xdata,p+14); attr=farbyte(xdata,p+28);
    w->number=n;
    w->prow=(tl>>8)+1; w->pcol=(tl&255)+1;
    w->nrows=(br>>8)-(tl>>8)+1; w->ncols=(br&255)-(tl&255)+1;
    w->vrow=(v>>8)+1; w->vcol=(v&255)+1;
    w->tracking=farbyte(xdata,p+30);
    w->display=farword(xdata,p+24)!=0xb000;
    w->foreground=attr&7; w->background=(attr>>4)&7;
    return 1;
}

/* XIOS normalizes geometry against the other windows. Bounds here describe
 * the public interface; XIOS remains responsible for overlap and clipping. */
w_apply(w) struct window *w;
{
    int p,a;
    if (w->prow<1 || w->prow>24 || w->pcol<1 || w->pcol>80 ||
        w->nrows<1 || w->nrows>24 || w->ncols<1 || w->ncols>80 ||
        w->vrow<1 || w->vrow>24 || w->vcol<1 || w->vcol>80) return 0;
    /* XIOS ww_new_window accepts raw corners without screen clipping.
     * Clamp the extent before passing it to the screen-map routines. */
    if(w->prow+w->nrows-1>24) w->nrows=25-w->prow;
    if(w->pcol+w->ncols-1>80) w->ncols=81-w->pcol;
    xcall(24,0,w->display,w->number);
    p=xcall(16,0,0,w->number);
    a=farword(xdata,p+28);
    a=(a&0xff00)|(w->foreground&7)|((w->background&7)<<4);
    farput(xdata,p+28,a);
    xcall(21,0,((w->vrow-1)<<8)|(w->vcol-1),
          (w->tracking<<8)|w->number);
    xcall(20,((w->prow+w->nrows-2)<<8)|(w->pcol+w->ncols-2),
          ((w->prow-1)<<8)|(w->pcol-1),w->number);
    return 1;
}
w_top(n) int n; { return xcall(19,0,3,n); }
w_full(n) int n; { return xcall(23,0,0,n); }
w_manager(state,n) int state,n; { return xcall(19,0,state,n); }
w_key() { return xcall(17,0,253,0); }

w_view()
{
    struct window w; int n; char line[128];
    message("Number PRow PCol NRows NCols VRow VCol Tracking Display FGColor BGColor\r\n");
    message("------ ---- ---- ----- ----- ---- ---- -------- ------- ------- -------\r\n");
    for(n=0;n<w_status(1);++n) {
        w_read(n,&w);
        format(line,"%4d %5d %4d %4d %5d %5d %4d %8s %7s %7s %7s\r\n",n,
          w.prow,w.pcol,w.nrows,w.ncols,w.vrow,w.vcol,
          w.tracking?"ROW":"NO",w.display?"COLOR":"B&W",
          colors[w.foreground],colors[w.background]);
        message(line);
    }
    return 0;
}

/* CP/M output uses complete 128-byte records and Ctrl-Z padding. */
static char fcb[36],record[128];
static int used,failed;
static flush_record()
{
    bdos(26,record);
    if ((bdos(21,fcb)&255)!=0) failed=1;
    used=0;
    return 0;
}
static out(c) int c;
{
    record[used++]=c;
    if (used==128) flush_record();
    return 0;
}
static text(s) char *s; { while(*s) out(*s++); return 0; }
static filename(s) char *s;
{
    int i,j,c;
    for(i=0;i<36;++i) fcb[i]=0;
    for(i=1;i<12;++i) fcb[i]=' ';
    if(s[0] && s[1]==':') { fcb[0]=s[0]-'A'+1; s+=2; }
    if(fcb[0]<0 || fcb[0]>16) return 0;
    j=1;
    while(*s && *s!='.') {
        c=*s++; if(j>8 || c<33 || c=='*' || c=='?' || c==':') return 0;
        fcb[j++]=c;
    }
    if(j==1) return 0;
    if(*s=='.') ++s;
    j=9;
    while(*s) {
        c=*s++; if(j>11 || c<33 || c=='*' || c=='?' || c==':') return 0;
        fcb[j++]=c;
    }
    return 1;
}
w_write(n,type,name) int n,type; char *name;
{
    struct window w; int p,seg,r,c,row,col,nr,nc;
    char line[160];
    if(!filename(name)) { message("Filename requested is not an acceptable filename.\r\n"); return 0; }
    if(!w_read(n,&w)) return 0;
    bdos(19,fcb);
    if((bdos(22,fcb)&255)==255) return 0;
    used=failed=0;
    if(type==2) {
        format(line,"window ch n=%d pr=%d,pc=%d,nr=%d,nc=%d,vr=%d,vc=%d,tr=%s,d=%s,fg=%s,bg=%s\r\n",
          n,w.prow,w.pcol,w.nrows,w.ncols,w.vrow,w.vcol,
          w.tracking?"row":"no",w.display?"color":"b&w",
          colors[w.foreground],colors[w.background]);
        text(line);
    } else {
        p=xcall(16,0,0,n); seg=farword(xdata,p+22);
        row=type==0?w.vrow-1:0; col=type==0?w.vcol-1:0;
        nr=type==0?w.nrows:24; nc=type==0?w.ncols:80;
        for(r=0;r<nr;++r) {
            for(c=0;c<nc;++c) out(farbyte(seg,2*((row+r)*80+col+c)));
            text("\r\n");
        }
    }
    if(used) { while(used<127) out(26); out(26); }
    if((bdos(16,fcb)&255)==255) failed=1;
    bdos(26,128);
    return !failed;
}
