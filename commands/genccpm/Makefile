# GENCCPM — DRI C system-generator build.
# Kernel composition and image generation belong to kernel/sysgen/Makefile.
# all builds the declared tools; check validates them; clean removes generated files.

CMDINFO=cmdinfo
DRC86=drccpm_cc
DRCLINK86=drccpm_link

TOOLS=genccpm.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

genccpm.cmd: genc1.obj genc2.obj genc3.obj genc4.obj genc5.obj genc6.obj Makefile
	$(DRCLINK86) genccpm[map]=genc1,genc2,genc3,genc4,genc5,genc6 [libsyms]

%.obj: %.c genccpm.h portab.h machine.h Makefile $(wildcard *.h)
	$(DRC86) -o$@ $<

clean:
	rm -f *.sys *.cmd *.obj *.h86 *.sym *.lin *.lst *.prn *.map

# Structural validation; byte comparisons or runtime checks are separate where available.
check: all
	@set -e; for file in $(TOOLS); do $(CMDINFO) $$file; done
