include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)

CFLAGS:=$(CFLAGS) -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"

BINARY=init

OBJS=\
init.o \

all: $(BINARY)

.PHONY: all install clean

$(BINARY): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BINARY) -lmount $(LIBS)

%.o: %.c
	$(CC) -std=gnu11 $(CFLAGS) $(CPPFLAGS) -c $< -o $@

install: all
	mkdir -p $(DESTDIR)$(SBINDIR)
	install $(BINARY) $(DESTDIR)$(SBINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	cp init.8 $(DESTDIR)$(MANDIR)/man8/init.8

clean:
	rm -f $(BINARY) $(OBJS) *.o
