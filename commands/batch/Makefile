# BATCH — annotated assembly reconstruction.
# batorg.a86 is the frozen reference; batch.a86 is the maintained source.
# all builds both CMDs; check validates complete files; clean removes build outputs.

ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
CMDINFO=cmdinfo
PYTHON=python3

TOOLS=batch.cmd batorg.cmd
REF=../../base/batch.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:
all: $(TOOLS)

# ASM86/GENCMD retain the original separate CODE and DATA groups. The private
# stack is inside DATA; no header normalization or binary patching is needed.
%.cmd: %.h86 Makefile
	$(GENCMD) $<
	$(CMDINFO) $@

%.h86: %.a86 Makefile
	$(ASM86) $<

# The frozen build must match base. check.py checks the maintained build against
# only the documented patch-area, USER/PAUSE and alias changes; all other bytes match.
check: $(TOOLS) check.py
	cmp batorg.cmd $(REF)
	$(PYTHON) check.py

clean:
	rm -f *.cmd *.obj *.h86 *.sym *.lst *.prn *.log
