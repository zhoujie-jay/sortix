include ../compiler.mak
include ../version.mak
include ../dirs.mak

OPTLEVEL?=-g -O2
CPPFLAGS?=
CXXFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS)
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES:=\
porttix-create \
srctix-create \
tix \
tix-build \
tix-collection \
tix-execdiff \
tix-execpatch \
tix-install \
tix-object-insert \
tix-rmpatch \

PROGRAMS:=\
$(BINARIES) \
tix-eradicate-libtool-la \

LIBS:=-lnettle

all: $(PROGRAMS)

.PHONY: all install clean

%: %.cpp util.h
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< -o $@ $(LIBS)

$(DESTDIR)$(BINDIR):
	mkdir -p $@

install: all $(DESTDIR)$(BINDIR)
	install $(PROGRAMS) $(DESTDIR)$(BINDIR)

clean:
	rm -f $(BINARIES)
