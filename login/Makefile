include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
CFLAGS:=$(CXXFLAGS) -Wall -Wextra

BINARY=login

OBJS=\
framebuffer.o \
graphical.o \
login.o \
pixel.o \
vgafont.o \

all: $(BINARY)

.PHONY: all install clean

$(BINARY): $(OBJS)
	$(CC) $(OBJS) -o $(BINARY) $(CXXFLAGS) $(LIBS)

%.o: %.c arrow.inc
	$(CC) -std=gnu11 $(CPPFLAGS) $(CFLAGS) -c $< -o $@

arrow.inc: arrow.rgb
	carray -cs --identifier=arrow arrow.rgb -o $@

install: all
	mkdir -p $(DESTDIR)$(SBINDIR)
	install $(BINARY) $(DESTDIR)$(SBINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	cp login.8 $(DESTDIR)$(MANDIR)/man8/login.8

clean:
	rm -f $(BINARY) $(OBJS) *.o arrow.inc
