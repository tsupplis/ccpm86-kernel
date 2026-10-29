# CONFIG — annotated assembly reconstruction.
# conforg.a86 is the frozen reference; config.a86 is the maintained source.
# all builds both CMDs; check validates complete files; clean removes build outputs.

ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=cmdinfo
PYTHON=python3

TOOLS=config.cmd conforg.cmd
REF=../../base/config.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

# Assemble absolute groups, then generate the complete CMD image.
# GENCMD inserts an empty EXTRA descriptor before STACK. header.py validates
# that layout and compacts descriptors without changing the group payloads.
%.cmd: %.h86 Makefile header.py
	$(GENCMD) $<
	$(PYTHON) header.py $@
	$(CMDINFO) $@

%.h86: %.a86 Makefile
	$(ASM86) $<

# Validate both the frozen reconstruction and maintained version, including headers.
check: $(TOOLS)
	cmp conforg.cmd $(REF)
	cmp config.cmd $(REF)
	@echo "PASS: both complete CMD files match $(REF)"

# Remove generated files only; the invariant source and base reference are retained.
clean:
	rm -f *.cmd *.h86 *.lst *.sym *.obj *.log
