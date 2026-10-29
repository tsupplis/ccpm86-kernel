# PUTGET — local command build.
# all builds the declared tools; check validates them; clean removes generated files.

CMDINFO=cmdinfo
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd

TOOLS=put.cmd get.cmd rdr.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

%.cmd: %.h86 Makefile
	$(GENCMD) $<
	$(CMDINFO) $@

%.h86: %.a86 putget.def putget.lib Makefile
	$(ASM86) $<

clean:
	rm -f *.sys *.cmd *.obj *.h86 *.sym *.lst *.prn *.loc *.lnk *.mp?

# Structural validation; byte comparisons or runtime checks are separate where available.
check: all
	@set -e; for file in $(TOOLS); do $(CMDINFO) $$file; done
