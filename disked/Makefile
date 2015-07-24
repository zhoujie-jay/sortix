SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)

CFLAGS:=$(CFLAGS) -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"

BINARY:=disked

all: $(BINARY)

.PHONY: all install clean

install: all
	mkdir -p $(DESTDIR)$(SBINDIR)
	install $(BINARY) $(DESTDIR)$(SBINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	cp disked.8 $(DESTDIR)$(MANDIR)/man8

$(BINARY): $(BINARY).c
	$(CC) $(CFLAGS) $(CPPFLAGS) -std=gnu11 $< -o $@ -lmount

clean:
	rm -f $(BINARY)
