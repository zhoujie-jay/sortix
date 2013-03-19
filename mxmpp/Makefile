include ../compiler.mak
include ../version.mak
include ../dirs.mak

ifndef OPTLEVEL
  OPTLEVEL=-g -O2
endif

ifndef CXXFLAGS
  CXXFLAGS:=$(OPTLEVEL)
endif

CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES=mxmpp

INSTALLBINARIES:=$(addprefix $(DESTDIR)$(BINDIR)/,$(BINARIES))

all: $(BINARIES)

.PHONY: all install uninstall clean

%: %.cpp
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(BINARIES)

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(INSTALLBINARIES)

