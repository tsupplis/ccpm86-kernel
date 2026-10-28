#include "win.h"
main()
{
    char tail[129]; int i,n;
    if(!w_init()) {
        message("WINDOW requires Concurrent CP/M system that supports windows.\r\n");
        return;
    }
    n=(*((char *)128)&255);
    if(n>127) n=127;
    for(i=0;i<n;++i) tail[i]=*((char *)129+i);
    tail[n]=0;
    w_command(tail);
}
