SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES:=sh sortix-sh

SORTIX_SH_SRCS=\
editline.cpp \
sh.cpp \
showline.cpp \
util.cpp

all: $(BINARIES)

.PHONY: all install clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)

sortix-sh: $(SORTIX_SH_SRCS) *.h
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $(SORTIX_SH_SRCS) -o $@

sh: proper-sh.cpp
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(BINARIES) *.o
