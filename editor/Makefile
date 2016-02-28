SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
CFLAGS:=$(CFLAGS) -Wall -Wextra

BINARY=editor

OBJS=\
command.o \
cursor.o \
display.o \
editor.o \
highlight.o \
input.o \
modal.o \
multibyte.o \
terminal.o \

all: $(BINARY)

.PHONY: all install clean

$(BINARY): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BINARY) $(LIBS)

%.o: %.c
	$(CC) -std=gnu11 $(CFLAGS) $(CPPFLAGS) -c $< -o $@

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARY) $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp editor.1 $(DESTDIR)$(MANDIR)/man1/editor.1
	mkdir -p $(DESTDIR)$(MANDIR)/man5
	cp editor.5 $(DESTDIR)$(MANDIR)/man5/editor.5

clean:
	rm -f $(BINARY) $(OBJS) *.o
