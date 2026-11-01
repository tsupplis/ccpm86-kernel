# DREDIX — local command build.
# all builds the declared tools; check validates them; clean removes generated files.
.PHONY: all check clean

CMDINFO=cmdinfo
DRC86=drccpm_cc
DRCLINK86=drccpm_link
RM=rm -f

TOOLS=dredix.cmd

all: $(TOOLS)

dredix.cmd: ed.obj common.obj ekey.obj ebuf.obj ecmdglb.obj ecmdio.obj epat.obj \
 emem.obj ecmdblk.obj ewv.obj ehelp.obj elesson.obj ewdv.obj ecmdsta.obj \
 vd.obj vk.obj dredix.inp Makefile
	$(DRCLINK86) '$(basename $@)[input]'
	$(CMDINFO) $@

dredix.inp: Makefile
	printf '%s\r\n' 'dredix=ed,' 'common,' 'ekey,' 'ebuf,' 'ecmdglb,' \
	  'ecmdio,' 'epat,' 'emem,' 'ecmdblk,' 'ewv,' 'ehelp,' 'elesson,' \
	  'ewdv,' 'ecmdsta,' 'vd,' 'vk' > $@

%.obj: %.c Makefile $(wildcard *.h)
	$(DRC86) -o$@ $<

clean:
	$(RM) *.obj *.lst *.sym *.cmd dredix.inp

# Structural validation; byte comparisons or runtime checks are separate where available.
check: all
	@set -e; for file in $(TOOLS); do $(CMDINFO) $$file; done
