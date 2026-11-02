# WINDOW — local command build.
# all builds the declared tools; check validates them; clean removes generated files.
.PHONY: all check clean
.DELETE_ON_ERROR:

CMDINFO=cmdinfo
RASM86=pcdev_rasm86
DRC86=drccpm_cc
DRCLINK86=drccpm_link
RM=rm -f
PYTHON=python3
LINK_ERRORS=undefined|unresolved|object file error|syntax error|unable|fatal

TOOLS=window.cmd wmenu.cmd

all: $(TOOLS)

window.cmd: window.obj start.obj command.obj win.obj format.obj xios.obj Makefile trim.py
	@$(DRCLINK86) window=start,window,command,win,format,xios > window-link.log 2>&1; result=$$?; cat window-link.log; test $$result -eq 0
	@if grep -qiE '$(LINK_ERRORS)' window-link.log; then exit 1; else test $$? -eq 1; fi
	$(PYTHON) trim.py $@
	$(CMDINFO) $@

livetest.cmd: livetest.obj start.obj command.obj win.obj format.obj xios.obj Makefile trim.py
	@$(DRCLINK86) livetest=start,livetest,command,win,format,xios > livetest-link.log 2>&1; result=$$?; cat livetest-link.log; test $$result -eq 0
	@if grep -qiE '$(LINK_ERRORS)' livetest-link.log; then exit 1; else test $$? -eq 1; fi
	$(PYTHON) trim.py $@
	$(CMDINFO) $@

wmenu.cmd: wmenu.obj start.obj win.obj format.obj xios.obj Makefile trim.py
	@$(DRCLINK86) wmenu=start,wmenu,win,format,xios > wmenu-link.log 2>&1; result=$$?; cat wmenu-link.log; test $$result -eq 0
	@if grep -qiE '$(LINK_ERRORS)' wmenu-link.log; then exit 1; else test $$? -eq 1; fi
	$(PYTHON) trim.py $@
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
