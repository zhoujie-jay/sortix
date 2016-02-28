include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)

SORTIXKERNEL=../kernel

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\" -I$(SORTIXKERNEL)/include -I.
CFLAGS:=$(CFLAGS) -Wall -Wextra

ifeq ($(HOST_IS_SORTIX),0)
  CPPFLAGS+=-D_GNU_SOURCE
endif

BINARIES=mkinitrd initrdfs

all: $(BINARIES)

.PHONY: all install clean

%: %.c crc32.c rules.c serialize.c zcrc32.c
	$(CC) -std=gnu11 $(CFLAGS) $(CPPFLAGS) $< crc32.c rules.c serialize.c zcrc32.c -o $@

clean:
	rm -f $(BINARIES)

install: all
	mkdir -p $(DESTDIR)$(SBINDIR)
	install $(BINARIES) $(DESTDIR)$(SBINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	cp initrdfs.8 $(DESTDIR)$(MANDIR)/man8/initrdfs.8
	cp mkinitrd.8 $(DESTDIR)$(MANDIR)/man8/mkinitrd.8
