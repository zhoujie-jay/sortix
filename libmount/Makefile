SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CFLAGS?=$(OPTLEVEL)

CFLAGS:=$(CFLAGS) -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -Iinclude

OBJS:=\
biosboot.o \
blockdevice.o \
devices.o \
crc32.o \
ext2.o \
extended.o \
filesystem.o \
gpt.o \
harddisk.o \
mbr.o \
partition.o \
uuid.o \

all: libmount.a

.PHONY: all clean install

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -std=gnu11 -c $< -o $@

libmount.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

install: libmount.a
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -RTv include $(DESTDIR)$(INCLUDEDIR)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp libmount.a $(DESTDIR)$(LIBDIR)

clean:
	rm -f libmount.a
	rm -f $(OBJS)
	rm -f *.o
