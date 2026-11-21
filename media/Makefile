PYTHON ?= python3
FORMAT = ccpm86-320
TWIST = $(PYTHON) ../tools/cpm86twist.py
CPMCP = cpmcp -f $(FORMAT)

.PHONY: all test check check-media check-qemu clean
.DELETE_ON_ERROR:
IMAGES = ccpm86.img ccpmtools.img ccpmext.img ccpmdev.img ccpmmaint.img ccpmnative.img ccpmdos.img
all: boottrack.bin $(IMAGES)

SYSTEM_COMMANDS = ../commands/chset/chset.cmd ../commands/date/date.cmd \
    ../commands/era/era.cmd \
    ../commands/help/help.cmd ../commands/helphlp/help.hlp \
    ../commands/ed/ed.cmd ../commands/ren/ren.cmd \
    ../commands/sdir/sdir.cmd ../commands/set/set.cmd ../commands/show/show.cmd \
    ../commands/submit/submit.cmd \
    ../commands/type/type.cmd \
    ../commands/sysdisk/sysdisk.cmd \
    ../commands/setdef/setdef.cmd ../commands/batch/batch.cmd

# Remaining original system utilities are selected explicitly.
BASE_SYSTEM_COMMANDS = ../base/pcetime.cmd

WINDOW_COMMANDS = ../commands/window/window.cmd ../commands/window/wmenu.cmd
WINDOW_SAMPLES = ../base/carddeck.sub ../base/clear.sub \
    ../base/glass.sub ../base/wordproc.sub

TOOLS_COMMANDS = ../commands/stop/stop.cmd ../commands/asm86/asm86.cmd ../commands/ddt86/ddt86.cmd \
    ../commands/gencmd/gencmd.cmd ../commands/genccpm/genccpm.cmd \
    ../commands/print/print.cmd ../commands/vcmode/vcmode.cmd \
    ../commands/tod/tod.cmd ../commands/era/eraq.cmd \
    ../commands/chkcpmfs/chkcpmfs.cmd \
    $(WINDOW_COMMANDS) $(WINDOW_SAMPLES)

# Extension disk: console redirection, DREDIX and FM.
EXT_COMMANDS = ../commands/putget/put.cmd ../commands/putget/get.cmd \
    ../commands/putget/rdr.cmd ../commands/dredix/dredix.cmd \
    ../commands/dredix/help.edx ../commands/dredix/lsn.edx ../commands/fm/fm.cmd

MAINT_COMMANDS = ../commands/dskmaint/dskmaint.cmd ../commands/dform/dform.cmd \
    ../commands/disk/dcopy.cmd ../commands/disk/dcomp.cmd \
    ../commands/hdmaint/hdmaint.cmd ../commands/initdir/initdir.cmd ../commands/tod/tod.cmd \
    ../commands/config/config.cmd ../commands/function/function.cmd ../commands/setup/setup.cmd ../commands/systat/systat.cmd ../commands/stop/stop.cmd

# Native and DOS development tools each occupy a data-only disk.
DEV_COMMANDS = ../dev/rasm86.cmd ../dev/link86.cmd ../dev/lib86.cmd \
    ../dev/xref86.cmd ../dev/sid86.cmd ../dev/cbas86.cmd ../dev/crun86.cmd \
    ../dev/pbasic.cmd ../dev/mbasic.cmd ../commands/asm80/asm80.cmd \
    ../commands/hexcom/hexcom.cmd

DOS_COMMANDS = ../dev/exe2bin.exe \
    ../dev/mslib.exe ../dev/mslink.exe ../dev/msmasm.exe ../dev/mbasic.com

# Commands and kernel are built by the top-level Makefile.
# This Makefile packages their outputs.

# DSKMAINT owns boot source assembly; media owns reserved-track construction.
../commands/dskmaint/bootsect.bin: ../commands/dskmaint/bootsect.a86 ../commands/dskmaint/Makefile
	$(MAKE) -C ../commands/dskmaint bootsect.bin

../commands/dskmaint/loader.bin: ../commands/dskmaint/loader.a86 ../commands/dskmaint/Makefile
	$(MAKE) -C ../commands/dskmaint loader.bin

boottrack.bin: ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin Makefile
	@test "$$(wc -c < ../commands/dskmaint/bootsect.bin)" -eq 147
	@test "$$(wc -c < ../commands/dskmaint/loader.bin)" -le 1792
	$(PYTHON) -c "import sys; sys.stdout.buffer.write(b'\xe5' * 4096)" > $@.tmp
	dd if=../commands/dskmaint/bootsect.bin of=$@.tmp bs=1 seek=0 conv=notrunc 2>/dev/null
	printf '\001' | dd of=$@.tmp bs=1 seek=511 conv=notrunc 2>/dev/null
	dd if=../commands/dskmaint/loader.bin of=$@.tmp bs=1 seek=2304 conv=notrunc 2>/dev/null
	mv $@.tmp $@

# On-demand empty physical fixture for existing tests.
ccpm86base.img: ../tools/mk320.py ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin diskdefs Makefile
	$(PYTHON) ../tools/mk320.py $@.logical.tmp --format ccpm86-320 \
	    --boot ../commands/dskmaint/bootsect.bin --loader ../commands/dskmaint/loader.bin --label "CCPM86  31"
	$(TWIST) twist $@.logical.tmp $@
	$(RM) $@.logical.tmp

ccpm86.img: ../tools/mk320.py ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin diskdefs startup.0 ../kernel/sysgen/ccpm.sys $(SYSTEM_COMMANDS) $(BASE_SYSTEM_COMMANDS) ../commands/pip/pip4.cmd Makefile
	$(PYTHON) ../tools/mk320.py $@.logical.tmp --format ccpm86-320 \
	    --boot ../commands/dskmaint/bootsect.bin --loader ../commands/dskmaint/loader.bin --label "CCPM86  31"
	$(CPMCP) $@.logical.tmp ../kernel/sysgen/ccpm.sys startup.0 $(SYSTEM_COMMANDS) $(BASE_SYSTEM_COMMANDS) 0:
	$(CPMCP) $@.logical.tmp ../commands/pip/pip4.cmd 0:PIP.CMD
	cpmls -F -f $(FORMAT) $@.logical.tmp '0:*.*'
	# System-disk fallback only finds interpreters marked SYS.
	cpmchattr -f $(FORMAT) $@.logical.tmp s 0:BATCH.CMD
	$(TWIST) twist $@.logical.tmp $@.tmp
	mv $@.tmp $@
	$(RM) $@.logical.tmp

ccpmtools.img: ../tools/mk320.py ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin diskdefs $(TOOLS_COMMANDS) ../commands/pip/pip4.cmd Makefile
	$(PYTHON) ../tools/mk320.py $@.logical.tmp --format ccpm86-320 \
	    --boot ../commands/dskmaint/bootsect.bin --loader ../commands/dskmaint/loader.bin --label "CCPM86  31"
	$(CPMCP) $@.logical.tmp $(TOOLS_COMMANDS) 0:
	$(CPMCP) $@.logical.tmp ../commands/pip/pip4.cmd 0:PIP.CMD
	cpmls -F -f $(FORMAT) $@.logical.tmp '0:*.*'
	$(TWIST) twist $@.logical.tmp $@.tmp
	mv $@.tmp $@
	$(RM) $@.logical.tmp

ccpmext.img: ../tools/mk320.py ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin diskdefs $(EXT_COMMANDS) ../commands/pip/pip4.cmd Makefile
	$(PYTHON) ../tools/mk320.py $@.logical.tmp --format ccpm86-320 \
	    --boot ../commands/dskmaint/bootsect.bin --loader ../commands/dskmaint/loader.bin --label "CCPM86  31"
	$(CPMCP) $@.logical.tmp $(EXT_COMMANDS) 0:
	$(CPMCP) $@.logical.tmp ../commands/pip/pip4.cmd 0:PIP.CMD
	cpmls -F -f $(FORMAT) $@.logical.tmp '0:*.*'
	$(TWIST) twist $@.logical.tmp $@.tmp
	mv $@.tmp $@
	$(RM) $@.logical.tmp

ccpmdev.img: ../tools/mk320.py ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin diskdefs $(DEV_COMMANDS) Makefile
	$(PYTHON) ../tools/mk320.py $@.logical.tmp --format ccpm86-320 \
	    --boot ../commands/dskmaint/bootsect.bin --loader ../commands/dskmaint/loader.bin --label "CCPM86  31"
	$(CPMCP) $@.logical.tmp $(DEV_COMMANDS) 0:
	cpmls -F -f $(FORMAT) $@.logical.tmp '0:*.*'
	$(TWIST) twist $@.logical.tmp $@.tmp
	mv $@.tmp $@
	$(RM) $@.logical.tmp

ccpmmaint.img: ../tools/mk320.py ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin diskdefs startup.0 ../kernel/sysgen/ccpm.sys $(MAINT_COMMANDS) ../commands/pip/pip4.cmd Makefile
	$(PYTHON) ../tools/mk320.py $@.logical.tmp --format ccpm86-320 \
	    --boot ../commands/dskmaint/bootsect.bin --loader ../commands/dskmaint/loader.bin --label "CCPM86  31"
	$(CPMCP) $@.logical.tmp ../kernel/sysgen/ccpm.sys startup.0 $(MAINT_COMMANDS) 0:
	$(CPMCP) $@.logical.tmp ../commands/pip/pip4.cmd 0:PIP.CMD
	cpmls -F -f $(FORMAT) $@.logical.tmp '0:*.*'
	$(TWIST) twist $@.logical.tmp $@.tmp
	mv $@.tmp $@
	$(RM) $@.logical.tmp

# Alternative native TMP system, installed under the normal boot filename.
ccpmnative.img: ../tools/mk320.py ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin diskdefs startup.0 ../kernel/sysgen/ccpm-native.sys $(SYSTEM_COMMANDS) $(BASE_SYSTEM_COMMANDS) ../commands/pip/pip4.cmd Makefile
	$(PYTHON) ../tools/mk320.py $@.logical.tmp --format ccpm86-320 \
	    --boot ../commands/dskmaint/bootsect.bin --loader ../commands/dskmaint/loader.bin --label "CCPM86  31"
	$(CPMCP) $@.logical.tmp ../kernel/sysgen/ccpm-native.sys 0:CCPM.SYS
	$(CPMCP) $@.logical.tmp startup.0 $(SYSTEM_COMMANDS) $(BASE_SYSTEM_COMMANDS) 0:
	$(CPMCP) $@.logical.tmp ../commands/pip/pip4.cmd 0:PIP.CMD
	# System-disk fallback only finds interpreters marked SYS.
	cpmchattr -f $(FORMAT) $@.logical.tmp s 0:BATCH.CMD
	$(TWIST) twist $@.logical.tmp $@.tmp
	mv $@.tmp $@
	$(RM) $@.logical.tmp

ccpmdos.img: ../tools/mk320.py ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin diskdefs $(DOS_COMMANDS) Makefile
	$(PYTHON) ../tools/mk320.py $@.logical.tmp --format ccpm86-320 \
	    --boot ../commands/dskmaint/bootsect.bin --loader ../commands/dskmaint/loader.bin --label "CCPM86  31"
	$(CPMCP) $@.logical.tmp $(DOS_COMMANDS) 0:
	$(TWIST) twist $@.logical.tmp $@.tmp
	mv $@.tmp $@
	$(RM) $@.logical.tmp

ccpmtest.img: ../tools/mk320.py ../commands/dskmaint/bootsect.bin ../commands/dskmaint/loader.bin diskdefs
	$(PYTHON) ../tools/mk320.py $@.logical.tmp --format ccpm86-320 \
	    --boot ../commands/dskmaint/bootsect.bin --loader ../commands/dskmaint/loader.bin --label "CCPM86  31"
	$(TWIST) twist $@.logical.tmp $@
	$(RM) $@.logical.tmp

# Check every packaged file, allowing only CP/M record padding after its bytes.
check-media: all
	$(MAKE) -C ../commands/dskmaint check
	$(MAKE) -C ../commands/disk check
	@set -e; for image in $(IMAGES); do \
	    $(TWIST) untwist $$image $$image.verify.tmp; \
	    case $$image in \
	      ccpm86.img) sources='../kernel/sysgen/ccpm.sys startup.0 $(SYSTEM_COMMANDS) $(BASE_SYSTEM_COMMANDS)' ;; \
	      ccpmtools.img) sources='$(TOOLS_COMMANDS)' ;; \
	      ccpmext.img) sources='$(EXT_COMMANDS)' ;; \
	      ccpmdev.img) sources='$(DEV_COMMANDS)' ;; \
	      ccpmdos.img) sources='$(DOS_COMMANDS)' ;; \
	      ccpmnative.img) sources='../kernel/sysgen/ccpm-native.sys startup.0 $(SYSTEM_COMMANDS) $(BASE_SYSTEM_COMMANDS)' ;; \
	      ccpmmaint.img) sources='../kernel/sysgen/ccpm.sys startup.0 $(MAINT_COMMANDS)' ;; \
	    esac; \
	    case $$image in \
	      ccpm86.img|ccpmnative.img) \
	        cpmls -A -f $(FORMAT) $$image.verify.tmp 0:batch.cmd | grep -q '^....s.... batch.cmd$$' ;; \
	    esac; \
	    for source in $$sources; do \
	      file=$$(basename $$source); \
	      if test "$$file" = ccpm-native.sys; then file=ccpm.sys; fi; \
	      $(CPMCP) $$image.verify.tmp 0:$$file $$image.file.tmp; \
	      head -c "$$(wc -c < $$source)" $$image.file.tmp | cmp $$source -; \
	      $(RM) $$image.file.tmp; \
	    done; \
	    if test $$image != ccpmdev.img && test $$image != ccpmdos.img; then \
	      $(CPMCP) $$image.verify.tmp 0:PIP.CMD $$image.file.tmp; \
	      cmp ../commands/pip/pip4.cmd $$image.file.tmp; \
	    fi; \
	    dd if=$$image of=$$image.boot.tmp bs=4096 count=1 2>/dev/null; \
	    cmp $$image.boot.tmp boottrack.bin; \
	    test "$$(wc -c < $$image)" -eq 327680; \
	    $(RM) $$image.verify.tmp $$image.file.tmp $$image.boot.tmp; \
	    echo "$$image: all payloads, boot track and size verified"; \
	done

check: test

check-qemu: check-media
	$(PYTHON) ../tools/boot_check.py

test: ccpm86.img ccpmtools.img
	./ccpm86

clean:
	$(RM) boottrack.bin boottrack.bin.tmp ccpm.sys ccpm86base.img $(IMAGES) ccpmtest.img *.img.*.tmp *.img.tmp
