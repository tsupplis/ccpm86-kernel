/* WINDOW command parser. Abbreviations follow the documented command and
 * field names. Parse a whole CHANGE before applying any XIOS modification.
 */
#include "win.h"
static char *command_names[]={"VIEW","TOP","FULL","WRITE","CHANGE"};
static char *field_names[]={"NUMBER","PROW","PCOL","NROWS","NCOLS","VROW","VCOL","TRACKING","DISPLAY","FGCOLOR","BGCOLOR","TYPE","FILENAME"};
static int minimum[]={1,2,2,2,2,2,2,2,1,2,2,1,2};
static char *cursor;
static skip() { while(*cursor==' ' || *cursor=='\t' || *cursor==',') ++cursor; return 0; }
static token(s) char *s;
{
    int n,c;
    skip(); n=0;
    while((c=*cursor)!=0 && c!='=' && c!=',' && c!=' ' && c!='\t') {
        ++cursor; if(n<79) s[n++]=c;
    }
    s[n]=0; return n;
}
static prefix(a,b,min) char *a,*b; int min;
{
    int n;
    for(n=0;*a && *a==*b;++n) { ++a; ++b; }
    return !*a && n>=min;
}
static lookup(s,table,count,mins) char *s,**table; int count,*mins;
{
    int i,found;
    found=-1;
    for(i=0;i<count;++i) if(prefix(s,table[i],mins?mins[i]:1)) {
        if(found>=0) return -1;
        found=i;
    }
    return found;
}
static number(s) char *s;
{
    int n;
    if(!*s) return -1;
    n=0;
    while(*s>='0' && *s<='9') { n=n*10+*s++-'0'; if(n>3276) return -1; }
    return *s?-1:n;
}
static help()
{
    message("Valid window commands are:\r\n  WINDOW VIEW\r\n  WINDOW TOP N=<0-3>\r\n  WINDOW FULL N=<0-3>\r\n");
    message("  WINDOW WRITE N=<0-3> Type=<Window, Console, or Setup>,File=<filename>\r\n");
    message("  WINDOW CHANGE N=<0-3> PRow=<1-24>,PCol=<1-80>\r\n");
    message("  WINDOW CHANGE N=<0-3> NRows=<1-24>,NCols=<1-80>\r\n");
    message("  WINDOW CHANGE N=<0-3> VRow=<1-24>,VCol=<1-80>\r\n");
    message("  WINDOW CHANGE N=<0-3> Tracking=<No or Row>,Display=<B&W or Color>\r\n");
    message("  WINDOW CHANGE N=<0-3> FGColor=<color>,BGColor=<color>\r\nAbbreviations are allowed.\r\n");
    return 0;
}
static error(s) char *s; { message(s); message("\r\n"); return 0; }
w_command(tail) char *tail;
{
    struct window w;
    char key[80],value[80],file[80];
    int values[13],seen[13],cmd,i,k,v,n,type;
    cursor=tail;
    for(i=0;tail[i];++i) if(tail[i]>='a' && tail[i]<='z') tail[i]-=32;
    if(!token(key)) { error("No window sub-command given."); help(); return 0; }
    cmd=lookup(key,command_names,5,0);
    if(cmd<0) { error("Not a valid window sub-command."); help(); return 0; }
    for(i=0;i<13;++i) { seen[i]=0; values[i]=0; }
    file[0]=0;
    while(token(key)) {
        k=lookup(key,field_names,13,minimum);
        skip();
        if(k<0 || *cursor++!='=' || !token(value)) return error("Could not interpret parameters in WINDOW CHANGE command.");
        seen[k]=1;
        if(k==12) { for(i=0;value[i];++i) file[i]=value[i]; file[i]=0; continue; }
        if(k==7) v=prefix(value,"NO",1)?0:prefix(value,"ROW",1)?1:-1;
        else if(k==8) v=prefix(value,"B&W",1)?0:prefix(value,"COLOR",1)?1:-1;
        else if(k==9 || k==10) {
            v=-1;
            for(i=0;i<8;++i) {
                char color[12]; int j;
                for(j=0;colors[i][j];++j) color[j]=colors[i][j]>='a'?colors[i][j]-32:colors[i][j];
                color[j]=0;
                if(prefix(value,color,1)) { if(v!=-1) {v=-1; break;} v=i; }
            }
        } else if(k==11) v=prefix(value,"WINDOW",1)?0:prefix(value,"CONSOLE",1)?1:prefix(value,"SETUP",1)?2:-1;
        else v=number(value);
        if(v<0) return error("Could not interpret parameters in WINDOW CHANGE command.");
        values[k]=v;
    }
    if(cmd==0) { w_view(); return 1; }
    n=values[0];
    if(!seen[0] || !w_read(n,&w)) return error("Sub-command requires NUMBER=<0-3>");
    if(cmd==1) { w_top(n); return 1; }
    if(cmd==2) { w_full(n); return 1; }
    if(cmd==3) {
        if(!seen[11]) return error("WINDOW WRITE command requires TYPE=<Window, Console, or Setup>");
        if(!seen[12]) return error("WINDOW WRITE command requires FILE=<filename>");
        if(!w_write(n,values[11],file)) return error("Error writing file, check disk.");
        return 1;
    }
    if(seen[11] || seen[12]) return error("Could not interpret parameters in WINDOW CHANGE command.");
    if(seen[1]) w.prow=values[1]; if(seen[2]) w.pcol=values[2];
    if(seen[3]) w.nrows=values[3]; if(seen[4]) w.ncols=values[4];
    if(seen[5]) w.vrow=values[5]; if(seen[6]) w.vcol=values[6];
    if(seen[7]) w.tracking=values[7]; if(seen[8]) w.display=values[8];
    if(seen[9]) w.foreground=values[9]; if(seen[10]) w.background=values[10];
    if(!w_apply(&w)) return error("Window coordinates are outside the acceptable range.");
    return 1;
}
