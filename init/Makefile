include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARY=init

OBJS=\
init.o \

all: $(BINARY)

.PHONY: all install clean

$(BINARY): $(OBJS)
	$(CXX) $(OBJS) -o $(BINARY) $(CXXFLAGS) -lmount $(LIBS)

%.o: %.c++
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

install: all
	mkdir -p $(DESTDIR)$(SBINDIR)
	install $(BINARY) $(DESTDIR)$(SBINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	cp init.8 $(DESTDIR)$(MANDIR)/man8/init.8

clean:
	rm -f $(BINARY) $(OBJS) *.o
