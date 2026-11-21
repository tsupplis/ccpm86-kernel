# CON — kernel module build.
# all builds local outputs; clean removes generated files.

ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
RASM86=pcdev_rasm86
LINK86=pcdev_linkcmd

SNIPPETS=bdos.con sup.con rtm.con cio.con sysdat.con xios.con xiosnat.con mem.con

all: $(SNIPPETS)

# Three independent XIOS modules; fixed ABI checked before publication.
XIOS_SOURCES=$(wildcard xios/*.a86 xios/*.inc xios/*.lib xios/*.json)
xios.con xiosnat.con: xios/pcdisks.lib $(XIOS_SOURCES) pcmode.lib ../../tools/build_xios.py
	python3 ../../tools/build_xios.py $(basename $@)
	@cmdinfo $@

bdos.obj: $(wildcard *.bdo) uda.cic xioscb.def
sup.h86: $(wildcard *.sup) patch.cod
rtm.h86: $(wildcard *.rtm) s8087.lib
cio.h86: $(wildcard *.cio) console.cio chario.cio uda.cic
mem.h86: $(wildcard *.mem)
sysdat.h86: sysdat.dat

bdos.con: bdos.obj
	$(LINK86) $@=$< '[data[origin[0]]]'
	@cmdinfo $@

%.con: %.h86
	$(GENCMD) $<
	mv $(basename $@).cmd $@
	@cmdinfo $@

%.h86: %.a86
	$(ASM86) $<

%.obj: %.a86
	$(RASM86) $<

clean:
	rm -rf .xios
	rm -f xios/pcdisks.lib
	rm -f *.obj *.h86 *.sym *.lst *.con *.cmd *.loc *.lnk *.lst


.PHONY: all clean
.DELETE_ON_ERROR:

# The shared disk definition remains in 128-byte records; P emits native
# 512-byte-sector DPBs. GENDEF owns the conversion, not the XIOS builder.
GENDEF_SOURCES=$(wildcard ../../commands/gendef/*.plm ../../commands/gendef/*.a86 ../../commands/gendef/*.plb ../../commands/gendef/*.lit) ../../commands/gendef/Makefile
../../commands/gendef/gendef.cmd: $(GENDEF_SOURCES)
	$(MAKE) -C ../../commands/gendef

xios/pcdisks.lib: xios/pcdisks.def ../../commands/gendef/gendef.cmd Makefile
	cd xios && emu2 ../../../commands/gendef/gendef.cmd pcdisks '$$BCP'
