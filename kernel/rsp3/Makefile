# RSP3 — kernel module build.
# all builds local outputs; clean removes generated files.

ASM86=cpm86_asm86
GENCMD=cpm86_gencmd

SNIPPETS= pctmp.rsp

all: $(SNIPPETS)

pctmp.h86: pcguard.lib pcshell.lib pcerror.lib pchelp.lib pcsearch.lib pcdata.lib

%.rsp: %.h86
	$(GENCMD) $<
	mv $(basename $@).cmd $@
	@cmdinfo $@

%.h86: %.a86
	$(ASM86) $<

clean:
	rm -f *.h86 *.sym *.lst *.rsp *.cmd

.PHONY: all clean
.DELETE_ON_ERROR:
