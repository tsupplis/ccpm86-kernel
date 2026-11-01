# PRINT — local command build.
# all builds the declared tools; check validates them; clean removes generated files.
.PHONY: all check clean

CMDINFO=cmdinfo
RASM86=pcdev_rasm86
DRC86=drccpm_cc
DRCLINK86=drccpm_link
RM=rm -f

TOOLS=print.cmd

all: $(TOOLS)

# sqlink.inp preserves the original module order; the linker supplies the C runtime.
print.cmd: main.obj sqprint.obj cpmif.obj cpyucase.obj cpmerr.obj crlf.obj fileopen.obj \
 firstch.obj getdrv.obj getnum.obj gettail.obj issysfil.obj itoa.obj nextfcb.obj okver.obj \
 parse.obj putfname.obj readsect.obj reverse.obj tlex.obj valid.obj sqlink.inp Makefile
	$(DRCLINK86) 'sqlink[input]'
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
