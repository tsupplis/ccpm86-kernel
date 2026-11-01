# WINDOW — local command build.
# all builds the declared tools; check validates them; clean removes generated files.
.PHONY: all check clean

CMDINFO=cmdinfo
RASM86=pcdev_rasm86
DRC86=drccpm_cc
DRCLINK86=drccpm_link
RM=rm -f

TOOLS=window.cmd wmenu.cmd
SHARED=start.obj command.obj win.obj format.obj xios.obj

all: $(TOOLS)

window.cmd: window.obj $(SHARED) Makefile
	$(DRCLINK86) window=start,window,command,win,format,xios
	$(CMDINFO) $@

livetest.cmd: livetest.obj $(SHARED) Makefile
	$(DRCLINK86) livetest=start,livetest,command,win,format,xios
	$(CMDINFO) $@

wmenu.cmd: wmenu.obj $(SHARED) Makefile
	$(DRCLINK86) wmenu=start,wmenu,command,win,format,xios
	$(CMDINFO) $@

%.obj: %.c Makefile $(wildcard *.h)
	$(DRC86) -o$@ $<

xios.obj: xios.a86 Makefile
	$(RASM86) $<

clean:
	$(RM) *.cmd *.obj hostcheck *.lst *.sym *.map *.log

check: all
	$${CC:-cc} -std=gnu89 -DHOST_TEST -Wno-int-conversion -Wno-deprecated-non-prototype win.c command.c hostcheck.c -o hostcheck
	./hostcheck
