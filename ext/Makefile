include ../compiler.mak
include ../version.mak
include ../dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS)
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

LIBS:=$(LIBS)

ifeq ($(HOST_IS_SORTIX),0)
  LIBS:=$(LIBS) -lfuse
  CPPFLAGS:=$(CPPFLAGS) -D_FILE_OFFSET_BITS=64
endif

BINARIES:=extfs

INSTALLBINARIES:=$(addprefix $(DESTDIR)$(BINDIR)/,$(BINARIES))

all: $(BINARIES)

.PHONY: all install uninstall clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(INSTALLBINARIES)

extfs: *.cpp *.h
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $(wildcard *.cpp) -o $@ $(LIBS)

clean:
	rm -f $(BINARIES) *.o

