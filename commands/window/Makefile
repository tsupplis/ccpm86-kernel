# WINDOW — DR C / RASM86 build.
# all builds local outputs; check validates them; clean removes generated files.

DRC86=drccpm_cc
RASM86=pcdev_rasm86
LINK86=drccpm_link
TOOLS=window.cmd wmenu.cmd
SHARED=start.obj command.obj win.obj format.obj xios.obj

.PHONY: all check clean
.DELETE_ON_ERROR:

all: $(TOOLS)

window.cmd: window.obj $(SHARED) Makefile
	@$(LINK86) window=start,window,command,win,format,xios > window-link.log 2>&1; result=$$?; cat window-link.log; test $$result -eq 0
	@! grep -iE "undefined|syntax error|unable" window-link.log

livetest.cmd: livetest.obj $(SHARED) Makefile
	@$(LINK86) livetest=start,livetest,command,win,format,xios > livetest-link.log 2>&1; result=$$?; cat livetest-link.log; test $$result -eq 0
	@! grep -iE "undefined|syntax error|unable" livetest-link.log

wmenu.cmd: wmenu.obj $(SHARED) Makefile
	@$(LINK86) wmenu=start,wmenu,command,win,format,xios > wmenu-link.log 2>&1; result=$$?; cat wmenu-link.log; test $$result -eq 0
	@! grep -iE "undefined|syntax error|unable" wmenu-link.log

# Compile C modules with local declarations.
%.obj: %.c win.h Makefile
	@$(DRC86) -o$@ $< > $*.log 2>&1; result=$$?; cat $*.log; test $$result -eq 0
	@! grep -iE "error|unable to open" $*.log

xios.obj: xios.a86 Makefile
	@$(RASM86) $< > xios.log 2>&1; result=$$?; cat xios.log; test $$result -eq 0
	@grep -qE 'Number of errors: +0\.' xios.log

check: all
	$${CC:-cc} -std=gnu89 -DHOST_TEST -Wno-int-conversion -Wno-deprecated-non-prototype win.c command.c hostcheck.c -o hostcheck
	./hostcheck

# Remove generated outputs only; preserve sources and reference files.
clean:
	$(RM) *.cmd *.obj hostcheck *.lst *.sym *.map *.log
