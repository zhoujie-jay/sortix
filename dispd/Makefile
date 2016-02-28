SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)

CFLAGS:=$(CFLAGS) -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -I include

CLIENT_OBJS:=\
client/library.o \
client/session.o \
client/window.o \

BINS:=client/libdispd.a

all: $(BINS)

.PHONY: all headers client clean install install-include-dirs \
        install-headers install-client-dirs install-client

headers:

client: client/libdispd.a

client/libdispd.a: $(CLIENT_OBJS)
	$(AR) rcs $@ $(CLIENT_OBJS)

clean:
	rm -f $(CLIENT_OBJS)
	rm -f $(BINS)
	rm -f *.o client/*.o

%.o: %.c
	$(CC) -std=gnu11 $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Installation into sysroot
install: install-headers install-client

install-include-dirs: headers
	mkdir -p $(DESTDIR)$(INCLUDEDIR)

install-headers: install-include-dirs headers
	cp -RTv include $(DESTDIR)$(INCLUDEDIR)

install-client-dirs:
	mkdir -p $(DESTDIR)$(LIBDIR)

install-client: install-client-dirs client
	cp -P client/libdispd.a $(DESTDIR)$(LIBDIR)
