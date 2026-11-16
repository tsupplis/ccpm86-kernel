# FM — file manager, Aztec CP/M-86 build and host regression checks.
.PHONY: all check check-host check-arch clean
.DELETE_ON_ERROR:
.NOTPARALLEL:
ACC=aztec43_acc
CMDINFO=cmdinfo
PYTHON=python3
CC=cc
# CP/M-86 small model, 8086 instructions, size optimisation; no math library.
CFLAGS=-target=cpm86 -Osize
HOSTFLAGS=-DHOST_TEST -std=c89 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-deprecated-non-prototype
OBJECTS=main.o ui.o model.o view.o control.o canvas.o native.o keys.o config.o browser.o locpick.o type.o metadata.o copy.o backup.o backupos.o mutate.o print.o run.o runos.o spawn.o spawnos.o fsnative.o fsmap.o xios.o

all: fm.cmd

fm.cmd: $(OBJECTS) Makefile
	$(ACC) $(CFLAGS) -o $@ $(OBJECTS)
	$(CMDINFO) $@

# Native adapters use Aztec assembler syntax and trailing-underscore symbols.
%.o: %.asm Makefile
	$(ACC) $(CFLAGS) -c $<

%.o: %.c fm.h location.h type.h fsnative.h browser.h locpick.h copy.h backup.h mutate.h config.h print.h run.h runos.h spawn.h Makefile
	$(ACC) $(CFLAGS) -c $<

viewtest: tests/views.c view.c model.c fm.h
	$(CC) $(HOSTFLAGS) -I. tests/views.c view.c model.c -o $@

typetest: tests/type.c type.c type.h location.h fm.h
	$(CC) $(HOSTFLAGS) -I. tests/type.c type.c -o $@

fstest: tests/fsnative.c fsnative.c fsnative.h copy.h location.h fm.h
	$(CC) $(HOSTFLAGS) -I. tests/fsnative.c fsnative.c mutate.c -o $@

browtest: tests/browser.c browser.c locpick.c type.c metadata.c copy.c ui.c model.c control.c fm.h browser.h location.h type.h copy.h
	$(CC) $(HOSTFLAGS) -I. tests/browser.c browser.c locpick.c type.c metadata.c copy.c backup.c mutate.c print.c run.c ui.c model.c control.c -o $@

copytest: tests/copy.c copy.c copy.h location.h fm.h
	$(CC) $(HOSTFLAGS) -I. tests/copy.c copy.c -o $@

copyfst: tests/copyfs.c fsnative.c fsnative.h copy.h fm.h
	$(CC) $(HOSTFLAGS) -I. tests/copyfs.c fsnative.c mutate.c -o $@

metatest: tests/metadata.c metadata.c location.h fm.h
	$(CC) $(HOSTFLAGS) -I. tests/metadata.c metadata.c -o $@

cfgtest: tests/config.c config.c ui.c model.c config.h location.h fm.h
	$(CC) $(HOSTFLAGS) -I. tests/config.c config.c ui.c model.c -o $@

keytest: tests/keys.c keys.c fm.h
	$(CC) $(HOSTFLAGS) -I. tests/keys.c keys.c -o $@

check-arch:
	$(PYTHON) tests/arch.py

laytest: tests/layout.c config.c ui.c model.c view.c control.c canvas.c fm.h config.h
	$(CC) $(HOSTFLAGS) -I. tests/layout.c config.c ui.c model.c view.c control.c canvas.c -o $@

check-host: spawntst backostst backtest screentst printtst runostst runtest mutatest laytest check-arch keytest cfgtest metatest copyfst copytest browtest viewtest typetest fstest
	./spawntst
	./backostst
	./backtest
	./screentst
	./printtst
	./runostst
	./runtest
	./mutatest
	./laytest
	./keytest
	./cfgtest
	./metatest
	./viewtest
	./typetest
	./fstest
	./browtest
	./copytest
	./copyfst

check: all type.o fsnative.o check-host

clean:
	$(RM) *.o *.obj *.cmd *.log *.sym *.map *.mp? spawntst viewtest typetest fstest browtest copytest copyfst metatest cfgtest keytest laytest mutatest runtest runostst printtst screentst backtest backostst

mutatest: tests/mutation.c mutate.c mutate.h location.h fm.h
	$(CC) $(HOSTFLAGS) -I. tests/mutation.c mutate.c -o $@

runtest: tests/run.c run.c run.h location.h fm.h
	$(CC) $(HOSTFLAGS) -I. tests/run.c run.c -o $@

runostst: tests/runos.c runos.c runos.h spawn.h fsnative.h
	$(CC) $(HOSTFLAGS) -I. tests/runos.c runos.c -o $@

printtst: tests/print.c print.c print.h location.h fm.h
	$(CC) $(HOSTFLAGS) -I. tests/print.c print.c -o $@

screentst: tests/screen.c native.c keys.c fm.h
	$(CC) $(HOSTFLAGS) -I. tests/screen.c native.c keys.c -o $@

backtest: tests/backup.c backup.c backup.h mutate.c
	$(CC) $(HOSTFLAGS) -I. tests/backup.c backup.c mutate.c -o $@

backostst: tests/backos.c backupos.c backup.h fsnative.h
	$(CC) $(HOSTFLAGS) -I. tests/backos.c backupos.c -o $@

spawntst: tests/spawn.c spawn.c spawn.h
	$(CC) $(HOSTFLAGS) -I. tests/spawn.c spawn.c -o $@
