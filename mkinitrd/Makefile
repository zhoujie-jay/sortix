include ../compiler.mak
include ../version.mak
include ../dirs.mak

ifndef OPTLEVEL
  OPTLEVEL=-g -O2
endif

ifndef CXXFLAGS
  CXXFLAGS:=$(OPTLEVEL)
endif

SORTIXKERNEL=../sortix
LIBC=../libc

CPPFLAGS:=$(CPPFLAGS) -I$(SORTIXKERNEL)/include
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES=mkinitrd initrdfs

INSTALLBINARIES:=$(addprefix $(DESTDIR)$(BINDIR)/,$(BINARIES))

all: $(BINARIES)

.PHONY: all install uninstall clean

%: %.cpp crc32.cpp rules.cpp $(LIBC)/ioleast.cpp
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< crc32.cpp rules.cpp $(LIBC)/ioleast.cpp -o $@

clean:
	rm -f $(BINARIES)

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(INSTALLBINARIES)

