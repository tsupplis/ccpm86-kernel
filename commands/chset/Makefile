# CHSET — local command build.
# all builds the declared tools; check validates them; clean removes generated files.

CMDINFO=cmdinfo
RASM86=pcdev_rasm86
DRC86=drccpm_cc
DRCLINK86=drccpm_link

TOOLS=chset.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

chset.cmd: main.obj chset.obj function.obj cpmif.obj Makefile
	$(DRCLINK86) chset[map]=main,chset,function,cpmif

cpmif.obj: cpmif.a86 Makefile
	$(RASM86) $<

%.obj: %.c Makefile $(wildcard *.h)
	$(DRC86) -o$@ $<
clean:
	rm -f *.sys *.loc *.cmd *.obj *.h86 *.sym *.lin *.lst *.prn ccpmtest.img
	rm -f *.loc *.mp? *.lnk *.map

# Structural validation; byte comparisons or runtime checks are separate where available.
check: all
	@set -e; for file in $(TOOLS); do $(CMDINFO) $$file; done
