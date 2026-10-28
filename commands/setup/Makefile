ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
TOOLS=setup.cmd setuporg.cmd
REF=../../base/setup.cmd
.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)
%.cmd: %.h86 Makefile
	$(GENCMD) $<
%.h86: %.a86
	$(ASM86) $<
check: $(TOOLS)
	cmp setuporg.cmd $(REF)
	cmp setup.cmd $(REF)
	@echo "PASS: both complete CMD files match $(REF)"
clean:
	rm -f *.cmd *.h86 *.lst *.sym *.obj *.log
