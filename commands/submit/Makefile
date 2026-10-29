# SUBMIT — local command build.
# all builds the declared tools; check validates them; clean removes generated files.

CMDINFO=cmdinfo
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd

TOOLS=submit.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

submit.cmd: submit.h86 Makefile
	$(GENCMD) $<
	$(CMDINFO) $@

%.h86: %.a86 Makefile
	$(ASM86) $<

clean:
	rm -f *.sys *.loc *.cmd *.obj *.h86 *.sym *.lin *.lst *.prn
	rm -f *.loc *.mp? *.lnk

# Structural validation; byte comparisons or runtime checks are separate where available.
check: all
	@set -e; for file in $(TOOLS); do $(CMDINFO) $$file; done
