# PRINT — native CCP/M C-source rebuild, with the CPMIF assembly interface.
# all compiles and links PRINT.CMD; check validates its CMD structure.

RASM86=pcdev_rasm86
DRC86=drccpm_cc
DRCLINK86=drccpm_link
CMDINFO=cmdinfo

TOOLS=print.cmd
MODULES=main sqprint cpmif cpyucase cpmerr crlf fileopen firstch getdrv getnum gettail issysfil itoa nextfcb okver parse putfname readsect reverse tlex valid
OBJECTS=$(addsuffix .obj,$(MODULES))
HEADERS=portab.h cpmfunc.h utildef.h

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

# sqlink.inp preserves the original module order; the C linker supplies its runtime.
print.cmd: $(OBJECTS) sqlink.inp Makefile
	$(DRCLINK86) 'sqlink[input]'
	@test -s $@
	$(CMDINFO) $@

cpmif.obj: cpmif.a86 Makefile
	$(RASM86) $<

%.obj: %.c $(HEADERS) Makefile $(wildcard *.h)
	$(DRC86) -o$@ $<

# PRINT is recompiled from C; there is no byte-identical assembly invariant.
check: $(TOOLS)
	$(CMDINFO) print.cmd

# Remove generated command, object, listing and linker outputs.
clean:
	$(RM) *.cmd *.obj *.lst *.sym *.map *.log
