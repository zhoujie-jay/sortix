SOFTWARE_MEANT_FOR_SORTIX=1
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

BINARIES:=\
pong \
conway \
snake \
asteroids \

INSTALLBINARIES:=$(addprefix $(DESTDIR)$(BINDIR)/,$(BINARIES))

LIBS:=-ldispd

all: $(BINARIES)

.PHONY: all install uninstall clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(INSTALLBINARIES)

%: %.cpp
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f $(BINARIES) *.o

