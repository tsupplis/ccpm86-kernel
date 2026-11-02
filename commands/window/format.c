/* Small formatter for the two conversions used by WINDOW/WMENU.
 * DRC's 16-bit small-model calling convention places all argument words on
 * the stack after fmt. Keeping this here avoids linking the general printf
 * conversion engine into these small utilities. */
format(dst,fmt) char *dst,*fmt;
{
    char **args,*s,*begin,*q,number[8];
    int width,len,n,negative,pad;
    args=(char **)(&fmt+1); begin=dst;
    while(*fmt) {
        if(*fmt!='%') { *dst++=*fmt++; continue; }
        ++fmt; width=0; pad=(*fmt=='0')?'0':' ';
        while(*fmt>='0' && *fmt<='9') width=width*10+*fmt++-'0';
        if(*fmt=='d') {
            n=(int)*args++; negative=n<0; if(negative) n=-n;
            q=number+7; *q=0;
            do { *--q='0'+n%10; n/=10; } while(n);
            if(negative) *--q='-'; s=q;
        } else if(*fmt=='s') s=*args++;
        else { *dst++=*fmt++; continue; }
        ++fmt;
        for(q=s,len=0;*q++;++len) ;
        while(width-->len) *dst++=pad;
        while(*s) *dst++=*s++;
    }
    *dst=0; return dst-begin;
}
