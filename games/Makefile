SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

CFLAGS:=$(CFLAGS) -Wall -Wextra
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES:=\
asteroids \
aquatinspitz \

LIBS:=-ldispd

all: $(BINARIES)

.PHONY: all install clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)

%: %.c
	$(CC) -std=gnu11 $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LIBS)

%: %.cpp
	$(CXX) -std=gnu++11 $(CXXFLAGS) $(CPPFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f $(BINARIES) *.o
