include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)

CFLAGS:=$(CFLAGS) -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"

ifeq ($(HOST_IS_SORTIX),0)
  CPPFLAGS+=-D_GNU_SOURCE
endif

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

%: %.c util.h
	$(CC) -std=gnu11 $(CFLAGS) $(CPPFLAGS) $< -o $@

$(DESTDIR)$(SBINDIR):
	mkdir -p $@

install: all $(DESTDIR)$(SBINDIR)
	install $(PROGRAMS) $(DESTDIR)$(SBINDIR)

clean:
	rm -f $(BINARIES)
