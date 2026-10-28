/* Run on Concurrent CP/M-86: exercise the GLASS command sequence against
 * the real assembly bridge and XIOS, then restore all windows and top VC.
 * This is a test program, not a distribution command.
 */
#include "win.h"
static char *glass[]={
 "CH n=0 prow=2,pcol=4,nrows=2,ncols=30,fg=black,bg=white",
 "CH n=1 prow=9,pcol=2,nrows=24,ncols=65",
 "CH n=2 prow=2,pcol=45,nrows=7,ncols=35,fg=black,bg=white",
 "CH n=3 prow=15,pcol=62,nrows=7,ncols=80,fg=black,bg=white"};
static int rows[]={2,9,2,15}, cols[]={4,2,45,62};
static int heights[]={2,16,7,7}, widths[]={30,65,35,19};
main()
{
    struct window saved[4],w;
    char command[100];
    int i,j,ok,top;
    if(!w_init() || w_status(1)!=4) {
        message("FAIL: four-window Concurrent system required\r\n"); return;
    }
    top=w_status(2); ok=1;
    for(i=0;i<4;++i) w_read(i,&saved[i]);
    for(i=0;i<4;++i) {
        for(j=0;glass[i][j];++j) command[j]=glass[i][j];
        command[j]=0;
        if(!w_command(command)) ok=0;
        if(!w_read(i,&w) || w.prow!=rows[i] || w.pcol!=cols[i] ||
           w.nrows!=heights[i] || w.ncols!=widths[i]) ok=0;
        if(i!=1 && (w.foreground!=0 || w.background!=7)) ok=0;
    }
    command[0]=0; format(command,"TOP n=%d",1);
    if(!w_command(command) || w_status(2)!=1) ok=0;
    for(i=0;i<4;++i) w_apply(&saved[i]);
    w_top(top);
    message(ok?"PASS: GLASS layout, colors and TOP; original state restored\r\n":
               "FAIL: GLASS layout or TOP; original state restored\r\n");
}
