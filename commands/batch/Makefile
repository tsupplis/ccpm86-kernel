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
	@python3 -c "from pathlib import Path; r=bytearray(Path('$(REF)').read_bytes()); r[0xc1:0x101]=bytes(64); assert Path('batch.cmd').read_bytes()==r, 'Unexpected BATCH difference outside zero-filled patch area'"
	@echo "PASS: invariant matches original; maintained build differs only in the 64-byte patch area"

clean:
	rm -f *.cmd *.obj *.h86 *.sym *.lst *.prn *.log
