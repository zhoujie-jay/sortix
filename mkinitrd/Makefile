include ../compiler.mak
include ../version.mak
include ../dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

SORTIXKERNEL=../kernel

CPPFLAGS:=$(CPPFLAGS) -I$(SORTIXKERNEL)/include -I.
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES=mkinitrd initrdfs

INSTALLBINARIES:=$(addprefix $(DESTDIR)$(BINDIR)/,$(BINARIES))

all: $(BINARIES)

.PHONY: all install uninstall clean

%: %.cpp crc32.cpp rules.cpp
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< crc32.cpp rules.cpp -o $@

clean:
	rm -f $(BINARIES)

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(INSTALLBINARIES)

