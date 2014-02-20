SOFTWARE_MEANT_FOR_SORTIX=1
include ../compiler.mak
include ../version.mak
include ../dirs.mak

OPTLEVEL?=-O2 -g
CXXFLAGS?=$(OPTLEVEL)
TESTDIR?=$(LIBEXECDIR)/test

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\" -DTESTDIR=\"$(TESTDIR)\"
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES:=\
regress \

TESTS:=\
test-pthread-basic \
test-pthread-once \
test-pthread-self \
test-pthread-tls \
test-signal-raise \

all: $(BINARIES) $(TESTS)

.PHONY: all install uninstall clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(TESTDIR)
ifneq ($(TESTS),)
	install $(TESTS) $(DESTDIR)$(TESTDIR)
endif

%: %.c++
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(BINARIES) $(TESTS) *.o

