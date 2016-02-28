SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)
TESTDIR?=$(LIBEXECDIR)/test

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\" -DTESTDIR=\"$(TESTDIR)\"
CFLAGS:=$(CFLAGS) -Wall -Wextra

BINARIES:=\
regress \

TESTS:=\
test-fmemopen \
test-pthread-argv \
test-pthread-basic \
test-pthread-main-join \
test-pthread-once \
test-pthread-self \
test-pthread-tls \
test-signal-raise \

all: $(BINARIES) $(TESTS)

.PHONY: all install clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(TESTDIR)
ifneq ($(TESTS),)
	install $(TESTS) $(DESTDIR)$(TESTDIR)
endif

%: %.c
	$(CC) -std=gnu11 $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -f $(BINARIES) $(TESTS) *.o
