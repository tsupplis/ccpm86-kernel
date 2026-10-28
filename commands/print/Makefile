RASM86=pcdev_rasm86
DRC86=drccpm_cc
DRCLINK86=drccpm_link

TOOLS=print.cmd
MODULES=main sqprint cpmif cpyucase cpmerr crlf fileopen firstch getdrv getnum gettail issysfil itoa nextfcb okver parse putfname readsect reverse tlex valid
OBJECTS=$(addsuffix .obj,$(MODULES))
HEADERS=portab.h cpmfunc.h utildef.h

.PHONY: all clean
.DELETE_ON_ERROR:
all: $(TOOLS)

print.cmd: $(OBJECTS) sqlink.inp Makefile
	$(DRCLINK86) 'sqlink[input]'

cpmif.obj: cpmif.a86
	$(RASM86) $<

%.obj: %.c $(HEADERS)
	$(DRC86) -o$@ $<

clean:
	rm -f $(OBJECTS) print.cmd $(addsuffix .lst,$(MODULES)) $(addsuffix .sym,$(MODULES)) print.map print.sym
