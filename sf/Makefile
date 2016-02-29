include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)

CFLAGS:=$(CFLAGS) -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"

BINARY:=sf

all: $(BINARY)

.PHONY: all install clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARY) $(DESTDIR)$(BINDIR)
	install sfnc $(DESTDIR)$(BINDIR)
	install sfncd $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp sf.1 $(DESTDIR)$(MANDIR)/man1/sf.1
	cp sfnc.1 $(DESTDIR)$(MANDIR)/man1/sfnc.1
	cp sfncd.1 $(DESTDIR)$(MANDIR)/man1/sfncd.1

$(BINARY): $(BINARY).c
	$(CC) $(CFLAGS) $(CPPFLAGS) -std=gnu11 $< -o $@

clean:
	rm -f $(BINARY) *.o
