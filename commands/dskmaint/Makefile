# DSKMAINT — local command build.
# all builds the declared tools; check validates them; clean removes generated files.

CMDINFO=cmdinfo
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
RASM86=pcdev_rasm86
LINK86=pcdev_linkcmd
H862BIN=h862bin
BIN2ASM=bin2asm

TOOLS=dskmaint.cmd dskmorg.cmd
IMAGES=bootsect.bin loader.bin
INCS=bootsect.inc loader.inc
REF=../../base/dskmaint.cmd

.PHONY: all check clean check-dsk check-live
.DELETE_ON_ERROR:
all: $(TOOLS) $(IMAGES)

dskmaint.h86: dskmaint.a86 $(INCS) Makefile
dskmorg.h86: dskmorg.a86 bootorg.inc loadorg.inc Makefile

# GENCMD creates all four historical groups, including the empty EXTRA
# descriptor and the initialized 256-byte STACK. XFFFF is in paragraphs.
dskmorg.cmd: CMDOPTS=extra[m200 xffff]
dskmaint.cmd: CMDOPTS=extra[m800 xffff]

dskmaint.cmd: dskmaint.h86 Makefile
	$(GENCMD) dskmaint '$(CMDOPTS)'
	$(CMDINFO) $@
dskmorg.cmd: dskmorg.h86 Makefile
	$(GENCMD) dskmorg '$(CMDOPTS)'
	$(CMDINFO) $@

%.h86: %.a86 Makefile
	unix2dos $<
	rm -f $@
	$(ASM86) $*
	@test -f $@

%.bin: %.h86
	$(H862BIN) $< $@

# bin2asm is already provided on PATH; labels belong to the including file.
%.inc: %.bin
	$(BIN2ASM) -m -f $@ $<
	unix2dos $@

# Frozen bytes are extracted only for the historical reconstruction.
bootorg.bin: $(REF)
	dd if=$(REF) of=$@ bs=1 skip=4554 count=147 2>/dev/null
loadorg.bin: $(REF)
	dd if=$(REF) of=$@ bs=1 skip=4701 count=846 2>/dev/null

check: check-dsk
check-dsk: dskmaint.cmd dskmorg.cmd $(IMAGES)
	cmp dskmorg.cmd $(REF)
	@echo 'PASS: CCP/M DSKMORG matches all groups, headers and padding'
check-live: check
	python3 -B tests/live.py

clean:
	rm -f *.cmd *.obj *.h86 *.lst *.sym *.log *.bin.boot *.mp? *.lnk
	rm -f $(IMAGES) $(INCS) bootorg.bin loadorg.bin bootorg.inc loadorg.inc
