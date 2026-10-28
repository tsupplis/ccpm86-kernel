ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
TOOLS=config.cmd conforg.cmd
REF=../../base/config.cmd
.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)
%.cmd: %.h86 Makefile header.py
	$(GENCMD) $<
	python3 header.py $@
%.h86: %.a86
	$(ASM86) $<
check: $(TOOLS)
	cmp conforg.cmd $(REF)
	cmp config.cmd $(REF)
	@echo "PASS: both complete CMD files match $(REF)"
clean:
	rm -f *.cmd *.h86 *.lst *.sym *.obj *.log
