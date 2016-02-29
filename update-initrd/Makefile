include ../build-aux/platform.mak
include ../build-aux/dirs.mak

all:

.PHONY: all install clean

clean:

install: all
	mkdir -p $(DESTDIR)$(SBINDIR)
	install update-initrd $(DESTDIR)$(SBINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	cp update-initrd.8 $(DESTDIR)$(MANDIR)/man8/update-initrd.8
