SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

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
	$(CXX) $(OBJS) -o $(BINARY) $(CXXFLAGS) $(LIBS)

%.o: %.c++
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARY) $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	install editor.1 $(DESTDIR)$(MANDIR)/man1/editor.1

clean:
	rm -f $(BINARY) $(OBJS) *.o
