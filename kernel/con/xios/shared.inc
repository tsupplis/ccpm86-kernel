; Shared XIOS constants, structure fields and instruction macros.
supmod                  equ     ds:word ptr .00h
dispatcher              equ     ds:word ptr .038h
mpmseg                  equ     ds:word ptr .040h
endseg                  equ     ds:word ptr .044h
nflags                  equ     ds:byte ptr .04Ah
sysdisk                 equ     ds:byte ptr .04Bh
tempdisk                equ     ds:byte ptr .050h
mdul                    equ     ds:word ptr .058h
mfl                     equ     ds:word ptr .05Ah
rlr                     equ     ds:word ptr .068h
thrdrt                  equ     ds:word ptr .072h
qlr                     equ     ds:word ptr .074h
version                 equ     ds:word ptr .078h
vernum                  equ     ds:word ptr .07Ah
mpmvernum               equ     ds:word ptr .07Ch
todday                  equ     ds:word ptr .07Eh
todhour                 equ     ds:byte ptr .080h
todmin                  equ     ds:byte ptr .081h
todsec                  equ     ds:byte ptr .082h
openvec                 equ     ds:word ptr .088h
owner8087               equ     ds:word ptr .08Ch
ndpintoff               equ     ds:word ptr .0A0h
ndpintseg               equ     ds:word ptr .0A2h
ndpvecoff               equ     ds:word ptr .0A4h
ndpvecseg               equ     ds:word ptr .0A6h
true                    equ     0ffffh
false                   equ     0
pdelay                  equ     141                              ;delay specified number of ticks
pdispatch               equ     142                              ;let other another process run
ppdadr                  equ     156                              ;dbl word ptr of process descriptor
pterm                   equ     143                              ;terminate process
devpoll                 equ     131                              ;poll device
devflagwait             equ     132                              ;flag wait
devflagset              equ     133                              ;flag set
tseconds                equ     155                              ;get date,hours,minutes,seconds
tickflag                equ     1                                ;tick flag number
secflag                 equ     2                                ;seconds flag number
minflag                 equ     3                                ;minutes flag number
fdcflag                 equ     4                                ;disk operation flag
ciflag                  equ     5                                ;console in flag
wwflag                  equ     6                                ;window manager flag
erflag                  equ     7                                ;get errror response
sin0flag                equ     8                                ;serial input port 0
sout0flag               equ     9                                ;serial output port 0
sin1flag                equ     10                               ;serial input port 1
sout1flag               equ     11                               ;serial output port 0
screenflag              equ     12                               ;graphics mode switch
lastflag                equ     12
plink                   equ     word ptr 0
pthread                 equ     word ptr plink + word
pstat                   equ     byte ptr pthread + word
pprior                  equ     byte ptr pstat + byte
pflag                   equ     word ptr pprior + byte
pname                   equ     byte ptr pflag + word
puda                    equ     word ptr pname + 8
pdisk                   equ     byte ptr puda + word
puser                   equ     byte ptr pdisk + byte
pmem                    equ     word ptr puser + 3
pparent                 equ     word ptr pmem + 8
pcons                   equ     byte ptr pparent + word
plist                   equ     byte ptr pcons + 4
pdlen                   equ     30H
psrun                   equ     0                                ;in ready list root
pspoll                  equ     1                                ;in poll list
psdelay                 equ     2                                ;in delay list
psswap                  equ     3                                ;in swap list
psterm                  equ     4                                ;terminating
pssleep                 equ     5                                ;sleep processing
psdq                    equ     6                                ;in dq list
psnq                    equ     7                                ;in nq list
psflagwait              equ     8                                ;in flag table
psciowait               equ     9                                ;waiting for character
pfsys                   equ     00001h                           ;system process
pfkeep                  equ     00002h                           ;do not terminate
pfkernal                equ     00004h                           ;resident in kernal
pfpure                  equ     00008h                           ;pure memory descibed
pftable                 equ     00010h                           ;from pd table
pfresource              equ     00020h                           ;waiting for resource
pfraw                   equ     00040h                           ;raw console i/o
pfctlc                  equ     00080h                           ;abort pending
pfactive                equ     00100h                           ;active tty
pftempkeep              equ     00200h                           ;don't terminate yet...
pfctld                  equ     00400h                           ;explicit detach occured
pfchildabort            equ     00800h                           ;child terminated abnormally
pfnoctls                equ     01000h                           ;control S not allowed
pfdskld                 equ     02000h                           ;process was loaded from disk
cowner                  equ     word ptr 00h
ceditflag               equ     byte ptr 04h
cflineedit              equ     040h                             ;CIO Function 10 is active
cmimic                  equ     byte ptr 08h
cstate                  equ     word ptr 0Eh
cmaxbufsiz              equ     word ptr 10h
ccblen                  equ     2ch
csmbuffered             equ     00001h
csmbackground           equ     00002h
csmpurging              equ     00004h
csmnoswitch             equ     00008h
csmsuspend              equ     00010h
csmabort                equ     00020h
csmfilefull             equ     00040h
csmctrlS                equ     00080h
csmctrlO                equ     00100h
csmctrlP                equ     00200h
mdlink                  equ     word ptr 0
mdstart                 equ     word ptr mdlink + word
mdlength                equ     word ptr mdstart + word
FWAIT                   equ     WAIT
tickspersecond          equ     60
picocwport              equ     020h                             ;port 0
picocwnseoi             equ     020h                             ;non specific end of interupt
picocwseoi              equ     060h                             ;specific end of interupt channel 0
diskchannel             equ     06h
keyboardchannel         equ     01h
timerchannel            equ     00h
nul                     equ     00h
soh                     equ     01h
stx                     equ     02h
etx                     equ     03h
eot                     equ     04h
enq                     equ     05h
ack                     equ     06h
bel                     equ     07h
bs                      equ     08h
ht                      equ     09h
lf                      equ     0ah
vt                      equ     0bh
ff                      equ     0ch
cr                      equ     0dh
so                      equ     0eh
shi                     equ     0fh
dle                     equ     10h
dc1                     equ     11h
dc2                     equ     12h
dc3                     equ     13h
dc4                     equ     14h
nak                     equ     15h
syn                     equ     16h
etb                     equ     17h
can                     equ     18h
em                      equ     19h
subb                    equ     1ah
esc                     equ     1bh
fs                      equ     1ch
gs                      equ     1dh
rds                     equ     1eh
us                      equ     1fh
del                     equ     7fh
xon                     equ     dc1
xoff                    equ     dc3
numvircons              equ     4                                ;number of virtual consoles
sysdat                  equ     ds:word ptr .0C06h
supervisoro             equ     ds:word ptr .0C08h
supervisors             equ     ds:word ptr .0C0Ah
supervisor              equ     dword ptr supervisoro
tick                    equ     ds:byte ptr .0C0Ch
tickssec                equ     ds:byte ptr .0C0Dh
door                    equ     ds:byte ptr .0C0Eh
npcns                   equ     ds:byte ptr .0C10h
nvcns                   equ     ds:byte ptr .0C11h
nccb                    equ     ds:byte ptr .0C12h
nlcb                    equ     ds:byte ptr .0C13h
ccb                     equ     ds:word ptr .0C14h
lcb                     equ     ds:word ptr .0C16h
dphtbl                  equ     ds:word ptr .0C18h
mddph                   equ     ds:word ptr .0C30h
genccpmbuf              equ     ds:word ptr .0C38h
debug                   equ     ds:byte ptr .0C3Ah
kbddata                 equ     060h                             ;input port for the key board data
kbdcontrol              equ     061h                             ;control port for the key board
ctrl                    equ     29
shftleft                equ     42
shftright               equ     54
alt                     equ     56
numlock                 equ     69
capslock                equ     58
ctrlbit                 equ     001h
shftleftbit             equ     002h
shftrightbit            equ     004h
altbit                  equ     008h
numlockbit              equ     010h
capslockbit             equ     020h
setflagbit              equ     040h                             ;0 = no set flag call, 1 = set flag
databit                 equ     080h                             ;0 = no data available, 1 = data
timer0reg               equ     040h                             ;first counter timer port
timer1reg               equ     041h
timer2reg               equ     042h
timercmndreg            equ     043h                             ;command port
timer60hz               equ     19886                            ;constant for 60.001 hz tick
timer1000hz             equ     533h                             ;constant for 1000 hz tone
beepcmnd                equ     10110110b                        ;timer 2 lsb,msb binary
beepon                  equ     03h
beepoff                 equ     0fch
porta                   equ     060h
portb                   equ     061h
portc                   equ     062h
ndpstatus               equ     word ptr 2                       ; status word in env_8087
resetflag               equ     word ptr .72h
wakekey                 equ     7DH                              ;window manager alarm clock
fullkey                 equ     7EH                              ;full/partial window toggle
pfklsize                equ     20                               ; chars per pfk in low table
pfkhsize                equ     4                                ; chars per pfk in high table
lowpfks                 equ     10                               ; number of 20 char pfk codes
highpfks                equ     40                               ; number of 4 char pfk codes
slmonobit               equ     01h                              ; if status goes to monochrome
slcolorbit              equ     02h                              ; if status goes to color
crtrows                 equ     24                               ; crt height
crtcols                 equ     80                               ; crt width
crtsize                 equ     crtrows * crtcols
crtfull                 equ     100H*(crtrows-1) + (crtcols-1)
sloffset                equ     crtsize                          ; status line offset
                        if      PCMODEXIOS
bufferrows              equ     25
                        endif
                        if      not PCMODEXIOS
bufferrows              equ     crtrows
                        endif
buffercells             equ     bufferrows * crtcols
buffersize              equ     buffercells * (numvircons*2 + 1)
monoseg                 equ     0B000h                           ; monochrome memory segment
colorseg                equ     0B800h                           ; color memory segment
monoport                equ     03B4h                            ; monochrome controller addr
colorport               equ     03D4h                            ; color controller addr
monocursor              equ     0C0Dh                            ; bottom two scan lines of MDA cell
colorcursor             equ     0607h                            ; color cursor code
compaqcursor            equ     0B0Ch                            ; hi-res cursor on color card
vrt                     equ     08h                              ; vertical retrace bit
hrt                     equ     01h                              ; horiz retrace bit
movsburst               equ     112                              ; string move per vert retrace
stosburst               equ     168                              ; string store per vert retrace
neg4                    equ     -4                               ; for reverse indexing
blank                   equ     0720h                            ; space
qmake                   equ     86h
qopen                   equ     87h
qread                   equ     89h
qwrite                  equ     8Bh
cursorstart             equ     10
cursorend               equ     11
displaystarthi          equ     12
displaystartlow         equ     13
cursorhi                equ     14
cursorlow               equ     15
normcount               equ     5
insync                  equ     01h
outsync                 equ     02h
horiz                   equ     word ptr 0[si]
vert                    equ     word ptr 2[si]
topcorners              equ     word ptr 4[si]
botcorners              equ     word ptr 6[si]
vscursor                equ     word ptr 0[bx]                   ; cursor row,column
vscurcol                equ     byte ptr 0[bx]
vscurrow                equ     byte ptr 1[bx]
vstopleft               equ     word ptr 2[bx]                   ; t l window corner
vsleft                  equ     byte ptr 2[bx]
vstop                   equ     byte ptr 3[bx]
vsbotright              equ     word ptr 4[bx]                   ; b r window corner
vsright                 equ     byte ptr 4[bx]
vsbottom                equ     byte ptr 5[bx]
vsoldtl                 equ     word ptr 6[bx]                   ; last t l corner
vsoldbr                 equ     word ptr 8[bx]                   ; last b r corner
vstextlast              equ     byte ptr 11[bx]                  ; inclusive logical bottom row
vscrtsize               equ     word ptr 10[bx]                  ; total rows, columns
vswinsize               equ     word ptr 12[bx]                  ; including invisible
vsviewpoint             equ     word ptr 14[bx]                  ; window top left
vsrows                  equ     word ptr 16[bx]                  ; window row count
vsrowsb                 equ     byte ptr 16[bx]
vscols                  equ     word ptr 18[bx]                  ; window column count
vscolsb                 equ     byte ptr 18[bx]
vscorrect               equ     word ptr 20[bx]                  ; char position factor
vsvcseg                 equ     word ptr 22[bx]                  ; vc map base segment
vscrtseg                equ     word ptr 24[bx]                  ; base of screen memory
vslistptr               equ     word ptr 26[bx]                  ; start of row updates
vsattrib                equ     byte ptr 28[bx]                  ; current char attrib
vsmode                  equ     byte ptr 29[bx]                  ; wrap, cursor on/off
vscurtrack              equ     byte ptr 30[bx]                  ; cursor tracking mode
vswidth                 equ     byte ptr 31[bx]                  ; wrap width
vsnumber                equ     byte ptr 32[bx]                  ; virt console num
vsbit                   equ     byte ptr 33[bx]                  ; vc num = bit pos
vssavecursor            equ     word ptr 34[bx]                  ; cursor save loc
vsvector                equ     word ptr 36[bx]                  ; console state vector
vsxlat                  equ     word ptr 38[bx]                  ; mono/color xlat table
vsnull                  equ     word ptr 40[bx]                  ; unused (up for grabs)
vstrueview              equ     word ptr 42[bx]                  ; corrected view point
vstruecol               equ     byte ptr 42[bx]
vstruerow               equ     byte ptr 43[bx]
vscurtype               equ     word ptr 44[bx]                  ; mono or color
vspfktbl                equ     word ptr 46[bx]                  ; prog func key base
vspfkptr                equ     word ptr 48[bx]                  ; pointer to pfk table
vspfkcount              equ     byte ptr 50[bx]                  ; pfk count down
vspfkexp                equ     byte ptr 51[bx]                  ; pfk expansion flag
vsmx                    equ     byte ptr 52[bx]                  ; mutual exclusion sema
vsbackcount             equ     byte ptr 53[bx]                  ; XIOS back door count
vsbackax                equ     word ptr 54[bx]                  ; ax for XIOS back door
vsbackbx                equ     word ptr 56[bx]
vsbackcx                equ     word ptr 58[bx]
vsbackdx                equ     word ptr 60[bx]
vsscreenmode            equ     byte ptr 62[bx]                  ; alpha/graphics mode
vsflagwait              equ     byte ptr 63[bx]                  ; if waiting for graphics
sizevs                  equ     64
wrapbit                 equ     01h                              ; wrap mode enabled
cursorbit               equ     02h                              ; cursor displayed
revbit                  equ     04h                              ; reverse video mode
syncbit                 equ     08h                              ; sync crt accesses
matchbit                equ     10h                              ; image matches physical
initmode                equ     wrapbit + cursorbit + matchbit
bytespersector          equ     512
sectorspertrack         equ     8                                ;1 to 8
bytespertrack           equ     sectorspertrack * bytespersector
fdcstat                 equ     03f4h                            ;status port for disk controller
fdcdata                 equ     fdcstat+1                        ;data port for disk controller
fdcport                 equ     03f2h                            ;all bits clear on channel reset
fdcon                   equ     00001100b                        ;mask to keep the 8272 unreset
fdcresetbit             equ     00000100b                        ;8272 reset when low
fdcnomotor              equ     11111100b                        ;mask for no motors
fdccmndread             equ     01100110b                        ;mfm, skip deleted data, read
fdccmndwrite            equ     01000101b                        ;mfm, write
fdccmndformat           equ     01001101b                        ;mfm, format
fdccmndseek             equ     00001111b                        ;seek
fdccmndrecal            equ     00000111b                        ;home to track 0
fdccmndsi               equ     00001000b                        ;sense interupt status
fdccmndspec             equ     00000011b                        ;specify
fdcreadywrite           equ     10000000b                        ;mask for transfer ready
fdcreadyread            equ     10000000b                        ;mask for transfer ready
fdcspec1                equ     11001111b                        ;srt=0c, hd unload=0f 1st specify byte
fdcspec2                equ     00000010b                        ;hd load=1, mode=dma 2nd specify byte
fbytes                  equ     2                                ;magic number for 512 bytes per sector
fsectors                equ     8                                ;sectors per track
fgap                    equ     050h                             ;this is IBM's, not August's
ffiller                 equ     0e5h                             ;fill character
rbytes                  equ     2                                ;magic number for 512 bytes
rsectors                equ     8
rgap                    equ     02ah
rdtl                    equ     0ffh
drive                   equ     byte ptr 14[bp]
mcnt                    equ     byte ptr 15[bp]
track                   equ     word ptr 12[bp]
sector                  equ     word ptr 10[bp]
dmaseg                  equ     word ptr 8[bp]
dmaoff                  equ     word ptr 6[bp]
xlt                     equ     0                                ;translation table offset in DPH
dpb                     equ     8                                ;disk parameter block offset in DPH
spt                     equ     0                                ;sectors per track offset in DPB
psh                     equ     15                               ;physical shift factor offset in DPB
dmac2address            equ     004h                             ;8237 channel 2 address		rw
dmac2count              equ     005h                             ;8237 channel 2 transfer count	rw
dmastatreg              equ     008h                             ;8237 status register		ro
dmabmskreg              equ     dmastatreg+2                     ;8237 binary channel mask	wo
dmamodereg              equ     dmastatreg+3                     ;8237 mode register		wo
dmacbpf                 equ     dmastatreg+4                     ;8237 clear byte ptr f/f	wo
dmapagefdc              equ     081h                             ;a16 to a20 for channel 2
dmamodewritefdc         equ     01001010b
dmamodereadfdc          equ     01000110b
dmamodeverifyfdc        equ     01000010b
dmabmskfdc              equ     00000010b                        ;binary channel mask for disk
udaerrmode              equ     byte ptr .10h                    ; error mode byte in uda
errmodecode             equ     0FEh                             ; or 0FFh for quick return
wpcode                  equ     03h                              ; ROS definition
maxretry                equ     4
submsgsize              equ     19
SLOWSEEK                equ     0                                ;if -1, forces 3 ms step rate
IBMSYSID                equ     0AA55h                           ;partition label id word
HDSYSID                 equ     0DBh                             ;(yours truly)
HDVFYERR                equ     0AAh                             ;artificial ROS error code
blocksize               equ     2048                             ;remap block size
IBMfn                   equ     ds:byte ptr .00h
IBMerrcode              equ     ds:byte ptr .01h
IBMlun                  equ     ds:byte ptr .02h
IBMhead                 equ     ds:byte ptr .03h
IBMcylinder             equ     ds:word ptr .04h
IBMsector               equ     ds:byte ptr .06h
IBMcount                equ     ds:byte ptr .07h
IBMxferofs              equ     ds:word ptr .08h
IBMxferseg              equ     ds:word ptr .0Ah
IBMXFERPTR              equ     dword ptr ibmxferofs
HDIOPBSIZE              equ     12
XTPTbootfl              equ     ds:byte ptr .00h
XTPTstarthead           equ     ds:byte ptr .01h
XTPTstartsector         equ     ds:byte ptr .02h
XTPTstartcylinder       equ     ds:byte ptr .03h
XTPTsystem              equ     ds:byte ptr .04h
XTPTendhead             equ     ds:byte ptr .05h
XTPTendsector           equ     ds:byte ptr .06h
XTPTendcylinder         equ     ds:byte ptr .07h
XTPTrelsect             equ     ds:word ptr .08h
XTPTsize                equ     ds:word ptr .0Ch
XTPTRECSIZE             equ     16
XTLBLcode               equ     ds:byte ptr .00h
XTLBLpart1              equ     ds:byte ptr .01BEh
XTLBLpart2              equ     ds:byte ptr .01CEh
XTLBLpart3              equ     ds:byte ptr .01DEh
XTLBLpart4              equ     ds:byte ptr .01EEh
XTLBLid                 equ     ds:word ptr .01FEh
HDLBchks                equ     ds:word ptr .00h
HDLBid                  equ     ds:byte ptr .02h
HDLBpstart              equ     ds:word ptr .021h
HDLBplength             equ     ds:word ptr .023h
HDLBnheads              equ     ds:byte ptr .025h
HDLBibmspt              equ     ds:byte ptr .026h
HDLBcpmoptions          equ     ds:byte ptr .027h
HDLBcpmnsect            equ     ds:byte ptr .028h
HDLBcpmdpb              equ     ds:byte ptr .029h
HDLBcpmspt              equ     ds:word ptr .029h
HDLBcpmbsh              equ     ds:byte ptr .02Bh
HDLBcpmblm              equ     ds:byte ptr .02Ch
HDLBcpmexm              equ     ds:byte ptr .02Dh
HDLBcpmdsm              equ     ds:word ptr .02Eh
HDLBcpmdrm              equ     ds:word ptr .030h
HDLBcpmal0              equ     ds:byte ptr .032h
HDLBcpmal1              equ     ds:byte ptr .033h
HDLBcpmcks              equ     ds:word ptr .034h
HDLBcpmoff              equ     ds:word ptr .036h
HDLBcpmpsh              equ     ds:byte ptr .038h
HDLBcpmprm              equ     ds:byte ptr .039h
HDLBcpmrsvd             equ     ds:word ptr .03Ah
HDLBcpmfudge            equ     ds:word ptr .03Eh
HDLBmapctl              equ     ds:byte ptr .040h
HDLBmapspb              equ     ds:word ptr .040h
HDLBmapmax              equ     ds:word ptr .042h
HDLBmapcnt              equ     ds:word ptr .044h
HDLBmapspareend         equ     ds:word ptr .046h
HDLBmaptable            equ     ds:word ptr .048h
HDLABELSIZE             equ     512
HDINFexists             equ     ds:byte ptr .00h
HDINFlogged             equ     ds:byte ptr .01h
HDINFfixed              equ     ds:byte ptr .02h
HDINFheads              equ     ds:byte ptr .03h
HDINFspt                equ     ds:byte ptr .04h
HDINFcyl                equ     ds:word ptr .05h
HDINFenddir             equ     ds:word ptr .07h
HDINFlabelofs           equ     ds:word ptr .09h
HDINFSIZE               equ     11
HDHWINTERRUPT           equ     0dh                              ;IBM controller interrupt
HDINTERRUPT             equ     13h                              ;IBM entry point for hard disk
HDPARMSINTERRUPT        equ     41h                              ;points to hard disk parms
HDFDINTERRUPT           equ     40h                              ;ROS revectors floppy io here
HDROSDATASEG            equ     40h
HDOPTIONADDR            equ     76h                              ;ibm controller option control
HDPTR                   equ     dword ptr HDINTERRUPT * 4
hdcontrolbyte           equ     byte ptr HDROSDATASEG*16 + HDOPTIONADDR
dphsize                 equ     20
                        if      PCMODEXIOS
pcscdirection           equ     word ptr 20[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcsccount               equ     word ptr 18[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcscfill                equ     word ptr 16[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcsctop                 equ     word ptr 14[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcscbottom              equ     word ptr 12[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcscfirst               equ     word ptr 10[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcsclast                equ     word ptr 8[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcscwidth               equ     word ptr 6[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcscrows                equ     word ptr 4[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcscsource              equ     word ptr 2[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
                        if      PCMODEXIOS
pcscstep                equ     word ptr 0[bp]
                        endif
                        if      not PCMODEXIOS
                        endif
divideinterrupt         equ     00h
singlestepinterrupt     equ     01h
nmiinterrupt            equ     02h
onebyteinterrupt        equ     03h
overflowinterrupt       equ     04h
clockinterrupt          equ     08h
keyboardinterrupt       equ     09h
hdhardinterrupt         equ     0Dh
diskinterrupt           equ     0Eh
equipint                equ     11h                              ; ROS equipment check int
memint                  equ     12h                              ; ROS memory check int
diskint                 equ     13h                              ; ROS disk int
asyncint                equ     14h                              ; ROS async port int
printint                equ     17h                              ; ROS printer int
tickinterrupt           equ     1Ch                              ; user tick vector
hdcodeinterrupt         equ     40h                              ; pass along to floppy
hdparaminterrupt        equ     41h                              ; hard disk param pointer
osinterrupt             equ     224                              ; normal CCP/M-86 entry
debuggerinterrupt       equ     225                              ; debugger entry to O.S.
flaginterrupt           equ     228                              ; to get an unused flag
fiddsinterrupt          equ     229                              ; for attachamatic drives
xiosinterrupt           equ     230                              ; for ver 1.0 back door
                        codemacro FNINIT
                        dw      0e3dbh
                        endm
                        codemacro FNDISI
                        dw      0e1dbh
                        endm
                        codemacro FNSTCW   dst:Mw
                        segfix  dst
                        db      0d9h
                        modrm   7,dst
                        endm
                        codemacro FNCLEX
                        dw      0e2dbh
                        endm
                        codemacro FNSTENV dst:M
                        segfix  dst
                        db      0d9h
                        modrm   6,dst
                        endm
                        codemacro FLDENV  src:M
                        db      9bh
                        segfix  src
                        db      0d9h
                        modrm   4,src
                        endm
