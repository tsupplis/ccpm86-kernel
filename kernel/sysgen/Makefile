# SYSGEN — kernel system-image generation.
# all builds local outputs; clean removes generated files.

SNIPPETS= ccpm.sys ccpm-native.sys

CONS=bdos.con cio.con mem.con rtm.con sup.con sysdat.con xios.con
RSPS=abort.rsp clock.rsp dir.rsp echo.rsp pin.rsp serin.rsp serout.rsp pctmp.rsp vout.rsp

all: $(SNIPPETS)


bdos.con: ../con/bdos.con
	@cp $< .

cio.con: ../con/cio.con
	@cp $< .

mem.con: ../con/mem.con
	@cp $< .

rtm.con: ../con/rtm.con
	@cp $< .

sup.con: ../con/sup.con
	@cp $< .

sysdat.con: ../con/sysdat.con
	@cp $< .

xios.con: ../con/xios.con
	@cp $< .

abort.rsp: ../rsp2/abort.rsp
	@cp $< .

clock.rsp: ../rsp2/clock.rsp
	@cp $< .

dir.rsp: ../rsp2/dir.rsp
	@cp $< .

echo.rsp: ../rsp2/echo.rsp
	@cp $< .

pin.rsp: ../rsp1/pin.rsp
	@cp $< .

serin.rsp: ../rsp1/serin.rsp
	@cp $< .

serout.rsp: ../rsp1/serout.rsp
	@cp $< .

pctmp.rsp: ../rsp3/pctmp.rsp
	@cp $< .

vout.rsp: ../rsp1/vout.rsp
	@cp $< .

ccpm.sys: ../../commands/genccpm/genccpm.cmd genccpm.in Makefile $(CONS) $(RSPS)
	@$(RM) cls.rsp sysdisk.rsp tmp.rsp
	@emu2 ../../commands/genccpm/genccpm.cmd < genccpm.in
	@cmdinfo ccpm.sys

# GENCCPM enumerates local RSPs: isolate the native selection from PCTMP.
NATIVE_CONS=$(filter-out xios.con,$(CONS))
NATIVE_RSPS=$(filter-out pctmp.rsp,$(RSPS))
ccpm-native.sys: ../../commands/genccpm/genccpm.cmd genccpm-native.in Makefile $(NATIVE_CONS) $(NATIVE_RSPS) ../con/xiosnat.con ../rsp1/tmp.rsp
	mkdir -p native
	cp $(NATIVE_CONS) $(NATIVE_RSPS) native/
	cp ../con/xiosnat.con native/xios.con
	cp ../rsp1/tmp.rsp native/tmp.rsp
	cd native && emu2 ../../../commands/genccpm/genccpm.cmd < ../genccpm-native.in
	cp native/ccpm.sys $@
	cmdinfo $@

clean:
	$(RM) native/*.con native/*.rsp native/*.sys native/*.old
	rm -f *.sys *.cmd *.obj *.h86 *.sym *.lin *.lst *.prn
	rm -f *.con *.rsp
	rm -f ccpm86.img ccpmtest.img

.PHONY: all clean
.DELETE_ON_ERROR:
