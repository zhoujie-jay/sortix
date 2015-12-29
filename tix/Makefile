include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
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
tix-rmpatch \

PROGRAMS:=\
$(BINARIES) \
tix-eradicate-libtool-la \

all: $(PROGRAMS)

.PHONY: all install clean

%: %.cpp util.h
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(DESTDIR)$(SBINDIR):
	mkdir -p $@

install: all $(DESTDIR)$(SBINDIR)
	install $(PROGRAMS) $(DESTDIR)$(SBINDIR)

clean:
	rm -f $(BINARIES)
