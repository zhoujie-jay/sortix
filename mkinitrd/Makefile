include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

SORTIXKERNEL=../kernel

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\" -I$(SORTIXKERNEL)/include -I.
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES=mkinitrd initrdfs

all: $(BINARIES)

.PHONY: all install clean

%: %.cpp crc32.cpp rules.cpp serialize.cpp zcrc32.c
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< crc32.cpp rules.cpp serialize.cpp zcrc32.c -o $@

clean:
	rm -f $(BINARIES)

install: all
	mkdir -p $(DESTDIR)$(SBINDIR)
	install $(BINARIES) $(DESTDIR)$(SBINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	cp initrdfs.8 $(DESTDIR)$(MANDIR)/man8/initrdfs.8
	cp mkinitrd.8 $(DESTDIR)$(MANDIR)/man8/mkinitrd.8
