# PRINT — DR C / RASM86 build.
# all builds local outputs; check validates them; clean removes generated files.

CMDINFO=cmdinfo
RASM86=pcdev_rasm86
DRC86=drccpm_cc
DRCLINK86=drccpm_link

TOOLS=print.cmd
MODULES=main sqprint cpmif cpyucase cpmerr crlf fileopen firstch getdrv getnum gettail issysfil itoa nextfcb okver parse putfname readsect reverse tlex valid
OBJECTS=$(addsuffix .obj,$(MODULES))
HEADERS=portab.h cpmfunc.h utildef.h

.PHONY: all check clean
.DELETE_ON_ERROR:

all: $(TOOLS)

# sqlink.inp preserves the original module order; the linker supplies the C runtime.
print.cmd: $(OBJECTS) sqlink.inp Makefile
	@$(DRCLINK86) 'sqlink[input]' > print-link.log 2>&1; result=$$?; cat print-link.log; test $$result -eq 0
	@! grep -qiE 'undefined|unresolved|syntax error|unable|fatal' print-link.log
	@test -s $@
	$(CMDINFO) $@

cpmif.obj: cpmif.a86 Makefile
	@$(RASM86) $< > cpmif.log 2>&1; result=$$?; cat cpmif.log; test $$result -eq 0
	@grep -qiE 'Number of errors: +0\.' cpmif.log

# Compile C modules with local declarations.
%.obj: %.c $(HEADERS) Makefile
	@$(DRC86) -o$@ $< > $*.log 2>&1; result=$$?; cat $*.log; test $$result -eq 0
	@! grep -qiE 'error|unable to open|fatal' $*.log

# Remove generated outputs only; preserve sources and reference files.
clean:
	$(RM) *.cmd *.obj *.lst *.sym *.map *.log

check: all
	$(CMDINFO) print.cmd
