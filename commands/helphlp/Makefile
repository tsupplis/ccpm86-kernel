# HELPHLP — indexed HELP database. Builds help.hlp from help.dat with the
# native HELP [C] indexer of ../help/help.cmd (run under emu2).

HELPCMD=../help/help.cmd

.PHONY: all check clean
.DELETE_ON_ERROR:

all: help.hlp

check: all

$(HELPCMD):
	$(MAKE) -C ../help help.cmd

# HELP's native indexer requires CRLF. Convert a temporary copy so the
# repository's text remains editable without rebuilding a malformed index.
# Tabs replace eight-space leading indents in that copy only; displayed
# columns are asserted identical, saving a block on the 320 KB system disk.
help.hlp: $(HELPCMD) help.dat Makefile
	@set -e; tmp=$$(mktemp -d); \
	trap 'rm -rf "$$tmp"' EXIT HUP INT TERM; \
	cp $(HELPCMD) "$$tmp/help.cmd"; \
	cp help.dat "$$tmp/help.dat"; \
	python3 -c "from pathlib import Path; import re; p=Path('$$tmp/help.dat'); b=p.read_bytes(); c=re.sub(rb'(?m)^( {8})+',lambda m:b'\t'*(len(m[0])//8),b); assert c.expandtabs(8)==b.expandtabs(8); p.write_bytes(c)"; \
	unix2dos "$$tmp/help.dat"; \
	(cd "$$tmp" && emu2 help.cmd '[C]') && cp "$$tmp/help.hlp" $@
	@test -s $@

clean:
	rm -f help.hlp
