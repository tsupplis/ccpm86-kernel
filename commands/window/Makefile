DRC86=drccpm_cc
RASM86=pcdev_rasm86
LINK86=drccpm_link
TOOLS=window.cmd wmenu.cmd
SHARED=start.obj command.obj win.obj format.obj xios.obj
OBJECTS=$(SHARED) window.obj wmenu.obj

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

window.cmd: window.obj $(SHARED) Makefile
	@$(LINK86) window=start,window,command,win,format,xios > window-link.log 2>&1; cat window-link.log
	@! grep -iE "undefined|syntax error|unable" window-link.log

livetest.cmd: livetest.obj $(SHARED) Makefile
	@$(LINK86) livetest=start,livetest,command,win,format,xios > livetest-link.log 2>&1; cat livetest-link.log
	@! grep -iE "undefined|syntax error|unable" livetest-link.log

wmenu.cmd: wmenu.obj $(SHARED) Makefile
	@$(LINK86) wmenu=start,wmenu,command,win,format,xios > wmenu-link.log 2>&1; cat wmenu-link.log
	@! grep -iE "undefined|syntax error|unable" wmenu-link.log

%.obj: %.c win.h
	@$(DRC86) -o$@ $< > $*.log 2>&1; cat $*.log
	@! grep -iE "error|unable to open" $*.log

xios.obj: xios.a86
	@$(RASM86) $< > xios.log 2>&1; cat xios.log
	@grep -qE 'Number of errors: +0\.' xios.log

check:
	$${CC:-cc} -std=gnu89 -DHOST_TEST -Wno-int-conversion -Wno-deprecated-non-prototype win.c command.c hostcheck.c -o hostcheck
	./hostcheck

clean:
	rm -f $(OBJECTS) $(TOOLS) livetest.obj livetest.cmd hostcheck *.lst *.sym *.log
