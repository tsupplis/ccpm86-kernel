ASM86=cpm86_asm86
GENCMD=cpm86_gencmd

TOOLS=function.cmd funcorg.cmd
REF=../../base/function.cmd

.PHONY: all check clean
all: $(TOOLS)

# This CCP/M version has separate CODE, DATA and STACK groups. ASM86/GENCMD
# preserve their exact lengths; LINK86 adds 40h bytes to the DATA group.
%.cmd: %.h86
	$(GENCMD) $<
	# GENCMD leaves an empty EXTRA descriptor before STACK. The original CMD
	# packs its descriptors; compact the header without changing any payload.
	@python3 -c "from pathlib import Path; p=Path('$@'); d=p.read_bytes(); h=d[:128]; g=b''.join(h[i:i+9] for i in range(0,72,9) if h[i]); p.write_bytes(g.ljust(72,b'\0')+h[72:]+d[128:])"
	@cmdinfo $@

%.h86: %.a86
	$(ASM86) $<

# funcorg is the invariant reconstruction. The maintained copy initially
# reproduces it too; compare whole files, including group headers and padding.
check: $(TOOLS)
	cmp funcorg.cmd $(REF)
	cmp function.cmd $(REF)
	@echo "PASS: both FUNCTION builds are byte-identical to $(REF)"

clean:
	rm -f *.cmd *.obj *.h86 *.sym *.lst *.prn *.log
