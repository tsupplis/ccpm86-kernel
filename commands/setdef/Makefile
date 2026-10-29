# SETDEF — local command build.
# all builds the declared tools; check validates them; clean removes generated files.

CMDINFO=cmdinfo
TOOLS=setdef.cmd

ASM86=cpm86_asm86
GENCMD=cpm86_gencmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

setdef.cmd: setdef.h86 Makefile
	$(GENCMD) $<
	$(CMDINFO) $@

setdef.h86: setdef.a86 Makefile
	$(ASM86) $<

clean:
	rm -f setdef.cmd setdef.h86 setdef.sym setdef.lst

# Structural validation; byte comparisons or runtime checks are separate where available.
check: all
	@set -e; for file in $(TOOLS); do $(CMDINFO) $$file; done
