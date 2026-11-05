# STOP - build the reconstructed PL/M command and the assembly reference.
ASM86=cpm86_asm86
GENCMD=cpm86_gencmd
RASM86=pcdev_rasm86
LINK86=pcdev_linkcmd
INTEL_PLM86=intel_plm86
PYTHON=python3
REF=tests/stop.ref

.PHONY: all check check-asm check-plm check-live clean
.DELETE_ON_ERROR:

all: stop.cmd

stoporg.h86: stoporg.a86 Makefile
	@$(ASM86) $< > stoporg.log 2>&1; result=$$?; cat stoporg.log; test $$result -eq 0
	@grep -qiE 'NUMBER OF ERRORS: +0' stoporg.log
	@test -s $@

stoporg.cmd: stoporg.h86 Makefile encodings.py
	$(GENCMD) $<
	$(PYTHON) encodings.py $@
	@test -s $@

# Assemble the shared 1984 startup without copying or modifying its source.
scd3.obj: scd3.a86 Makefile
	@set -e; tmp=$$(mktemp -d); \
	  trap 'rm -f "$$tmp/scd3.a86" "$$tmp/scd3.obj" "$$tmp/scd3.lst" "$$tmp/scd3.sym"; rmdir "$$tmp"' EXIT HUP INT TERM; \
	  ln -s "$(abspath $<)" "$$tmp/scd3.a86"; \
	  (cd "$$tmp" && $(RASM86) scd3.a86) > scd3.log 2>&1 || { cat scd3.log; exit 1; }; \
	  cat scd3.log; grep -qiE 'Number of errors: +0\.' scd3.log; \
	  test -s "$$tmp/scd3.obj"; mv "$$tmp/scd3.obj" $@

stop.obj: stop.plm Makefile
	@$(INTEL_PLM86) $< debug 'optimize(3)' xref > stop.log 2>&1; \
	  result=$$?; cat stop.log; test $$result -eq 0
	@grep -qiE 'COMPILATION COMPLETE[.[:space:]]+[0-9]+ WARNINGS,[[:space:]]+0 ERRORS' stop.log
	@test -s $@

stop.cmd: scd3.obj stop.obj Makefile
	@$(LINK86) 'stop=scd3,stop[data[origin[0],max[0]]]' > stop-link.log 2>&1; \
	  result=$$?; cat stop-link.log; test $$result -eq 0
	@! grep -qiE 'undefined|unresolved|error|unable|cannot|invalid|fatal' stop-link.log
	@test -s $@

check: all check-asm check-plm

check-asm: stoporg.cmd $(REF)
	cmp stoporg.cmd $(REF)

check-plm: stop.cmd tests/stopcheck.asm tests/stop_check.py $(REF)
	$(PYTHON) tests/stop_check.py $(REF) stop.cmd

check-live: stop.cmd tests/live.py
	$(PYTHON) tests/live.py

clean:
	$(RM) stop.cmd stop.obj stop.log stop.lst stop.lin stop.sym stop-link.log
	$(RM) stoporg.cmd stoporg.h86 stoporg.log stoporg.lst stoporg.sym
	$(RM) scd3.obj scd3.log
