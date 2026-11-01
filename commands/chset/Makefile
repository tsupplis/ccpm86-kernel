# CHSET — local command build.
# all builds the declared tools; check validates them; clean removes generated files.
.PHONY: all check clean

CMDINFO=cmdinfo
RASM86=pcdev_rasm86
DRC86=drccpm_cc
DRCLINK86=drccpm_link
RM=rm -f

TOOLS=chset.cmd

all: $(TOOLS)

chset.cmd: main.obj chset.obj function.obj cpmif.obj Makefile
	$(DRCLINK86) chset[map]=main,chset,function,cpmif
	$(CMDINFO) $@

cpmif.obj: cpmif.a86 Makefile
	$(RASM86) $<

%.obj: %.c Makefile $(wildcard *.h)
	$(DRC86) -o$@ $<

clean:
	$(RM) *.sys *.loc *.cmd *.obj *.h86 *.sym *.lin *.lst *.prn ccpmtest.img *.log
	$(RM) *.loc *.mp? *.lnk *.map

# Structural validation; byte comparisons or runtime checks are separate where available.
check: all
	@set -e; for file in $(TOOLS); do $(CMDINFO) $$file; done
