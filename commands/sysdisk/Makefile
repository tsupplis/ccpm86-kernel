# SYSDISK — local command build.
# all builds the declared tools; check validates them; clean removes generated files.

CMDINFO=cmdinfo
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd

TOOLS=sysdisk.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

sysdisk.cmd: sysdisk.h86 Makefile
	$(GENCMD) $<
	$(CMDINFO) $@

check: sysdisk.cmd sysdisk.ref
	cmp sysdisk.cmd sysdisk.ref
	@echo "SYSDISK: byte-identical to sysdisk.ref"

%.h86: %.a86 Makefile
	$(ASM86) $<

clean:
	rm -f *.sys *.loc *.cmd *.obj *.h86 *.sym *.lin *.lst *.prn ccpmtest.img
	rm -f *.loc *.mp? *.lnk
