include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

KBLAYOUTDIR?=$(DATAROOTDIR)/kblayout

KBLAYOUT_COMPILER?=kblayout-compiler

LAYOUTS=\
dk \
us \

.PHONY: all install clean

all: $(LAYOUTS)

%: %.kblayout
	$(KBLAYOUT_COMPILER) --format=sortix-kblayout-1 --compression=none $< -o $@

install: all
	mkdir -p $(DESTDIR)$(KBLAYOUTDIR)
	cp $(LAYOUTS) $(DESTDIR)$(KBLAYOUTDIR)

clean:
	rm -f $(LAYOUTS)
