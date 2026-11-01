# WINDOW — local command build.
# all builds the declared tools; check validates them; clean removes generated files.
.PHONY: all check clean

CMDINFO=cmdinfo
RASM86=pcdev_rasm86
DRC86=drccpm_cc
DRCLINK86=drccpm_link
RM=rm -f

TOOLS=window.cmd wmenu.cmd

all: $(TOOLS)

window.cmd: window.obj start.obj command.obj win.obj format.obj xios.obj Makefile
	$(DRCLINK86) window=start,window,command,win,format,xios
	$(CMDINFO) $@

livetest.cmd: livetest.obj start.obj command.obj win.obj format.obj xios.obj Makefile
	$(DRCLINK86) livetest=start,livetest,command,win,format,xios
	$(CMDINFO) $@

wmenu.cmd: wmenu.obj start.obj command.obj win.obj format.obj xios.obj Makefile
	$(DRCLINK86) wmenu=start,wmenu,command,win,format,xios
	$(CMDINFO) $@

xios.obj: xios.a86 Makefile
	$(RASM86) $<

%.obj: %.c Makefile $(wildcard *.h)
	$(DRC86) -o$@ $<

clean:
	$(RM) *.sys *.loc *.cmd *.obj *.h86 *.sym *.lin *.lst *.prn ccpmtest.img *.log
	$(RM) *.loc *.mp? *.lnk *.map
	$(RM) hostcheck

# Structural validation; byte comparisons or runtime checks are separate where available.
check: all
	@set -e; for file in $(TOOLS); do $(CMDINFO) $$file; done
	$${CC:-cc} -std=gnu89 -DHOST_TEST -Wno-int-conversion -Wno-deprecated-non-prototype win.c command.c hostcheck.c -o hostcheck
	./hostcheck
