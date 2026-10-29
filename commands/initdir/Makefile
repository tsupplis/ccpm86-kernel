# INITDIR — PL/I compilation, object conversion and RASM-86 support modules.
# all builds the declared tools; check validates them; clean removes generated files.

CMDINFO=cmdinfo
RASM86=pcdev_rasm86
PLI86=drplicpm_pc
PLILINK86=drplicpm_link
PLIOBJFIX=drpli_objfix

TOOLS=initdir.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

check: all
	python3 ../../tools/initdir_check.py

initdir.cmd: initdir.obj initdira.obj diomod.obj Makefile
	$(PLILINK86) initdir [locals],initdira,diomod
	$(CMDINFO) $@

# Recompile after declarations or the object converter change. Old converted
# objects cannot recover the reserved DATA storage removed by a faulty tool.
initdir.obj: initdir.pli initdira.dcl diomod.dcl Makefile $(shell command -v $(PLIOBJFIX))
	$(PLI86) $< \$$dl
	$(PLIOBJFIX) $@

diomod.obj: diomod.a86 Makefile
	$(RASM86) $<

initdira.obj: initdira.a86 Makefile
	$(RASM86) $<

clean:
	rm -f *.sys *.loc *.cmd *.obj *.h86 *.sym *.lin *.lst *.prn ccpmtest.img
	rm -f *.loc *.mp? *.lnk
