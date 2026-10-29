# Build modules and RSPs before assembling the system images.
SUBDIRS=con rsp1 rsp2 rsp3 sysgen


all clean:
	set -e; for d in $(SUBDIRS); do \
		$(MAKE) -C $$d $@; \
	done

.PHONY: all clean
