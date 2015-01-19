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

%: %.cpp crc32.cpp rules.cpp serialize.cpp
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< crc32.cpp rules.cpp serialize.cpp -o $@

clean:
	rm -f $(BINARIES)

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)
