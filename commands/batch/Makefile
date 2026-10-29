ASM86=cpm86_asm86
GENCMD=cpm86_gencmd

TOOLS=batch.cmd batorg.cmd
REF=../../base/batch.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

# ASM86/GENCMD retain the original separate CODE and DATA groups. The private
# stack is inside DATA; no header normalization or binary patching is needed.
%.cmd: %.h86
	$(GENCMD) $<
	@cmdinfo $@

%.h86: %.a86
	$(ASM86) $<

# Compare the entire file, including descriptors, patch areas and padding.
check: $(TOOLS)
	cmp batorg.cmd $(REF)
	python3 check.py

clean:
	rm -f *.cmd *.obj *.h86 *.sym *.lst *.prn *.log
