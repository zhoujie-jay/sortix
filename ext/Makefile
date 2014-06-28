include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

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

all: $(BINARIES)

.PHONY: all install clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)

extfs: *.cpp *.h
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) *.cpp -o $@ $(LIBS)

clean:
	rm -f $(BINARIES) *.o
