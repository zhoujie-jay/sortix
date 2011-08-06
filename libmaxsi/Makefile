ifndef CPU
    CPU=x86
endif

ifeq ($(CPU),x86)
    CPUDEFINES=-DPLATFORM_X86
    CPUFLAGS=-m32
    CPULDFLAGS=-melf_i386
    CPUASFLAGS=-32
    CPUNASMFLAGS=-felf32
endif

ifeq ($(CPU),x64)
    CPU=x64
    CPUDEFINES=-DPLATFORM_X64
    CPUFLAGS=-fPIC -m64
    CPULDFLAGS=-melf_x86_64
    CPUASFLAGS=-64
    CPUNASMFLAGS=-felf64
endif

DEFINES=-DLIBMAXSI_LIBRARY -DSORTIX $(CPUDEFINES)
FLAGSRELEASE=-O2
FLAGSDEBUG=-O2
FLAGS=$(CPUFLAGS) -std=gnu++0x -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-stack-protector -nostdinc $(FLAGSRELEASE) $(DEFINES)
LDFLAGS=$(CPULDFLAGS)
ASFLAGS=$(CPUASFLAGS)
NASMFLAGS=$(CPUNASMFLAGS)

COBJS=c/file.o
CHEADERS=\
c/h/unistd.h \
c/h/stdlib.h \
c/h/wchar.h \
c/h/stddef.h \
c/h/fcntl.h \
c/h/stdarg.h \
c/h/wctype.h \
c/h/features.h \
c/h/string.h \
c/h/sys/stat.h \
c/h/sys/types.h \
c/h/stdio.h

COMMONOBJS=c++.o thread.o io.o memory.o string.o error.o format.o
SORTIXOBJS:=$(addprefix sortix/,$(COMMONOBJS))
LIBMAXSIOBJS:=$(COMMONOBJS)
HEADERS=error.h io.h memory.h platform.h string.h syscall.h thread.h types.h format.h

OBJS:=$(LIBMAXSIOBJS)
BINS:=libmaxsi.so

ifndef LIBMAXSI_NO_STATIC
    BINS:=$(BINS) libmaxsi.a
endif

ifndef LIBMAXSI_NO_SORTIX
    BINS:=$(BINS) libmaxsi-sortix.a start.o
endif

ifndef LIBMAXSI_NO_LIBC
    OBJS:=$(OBJS) $(COBJS)
    BINS:=$(BINS) libc.so
    ifndef LIBMAXSI_NO_STATIC
         BINS:=$(BINS) libc.a
    endif

    HEADERS:=$(HEADERS) $(CHEADERS)

else
    DEFINES:=$(DEFINES) -DLIBMAXSI_NO_LIBC
endif

CFLAGS:=$(FLAGS) -std=c99 -Ic/h -I..
CXXFLAGS:=$(FLAGS) -nostdinc++ -fno-rtti -I.. -Ic/h
SORTIXFLAGS:=$(CXXFLAGS) -DSORTIX_KERNEL -I..  -I../..

all: $(BINS)

libmaxsi.a: $(OBJS)
	ar rcs libmaxsi.a $(OBJS)

libmaxsi.so: $(OBJS)
	ld $(LDFLAGS) -shared -o $@ $(OBJS)

libmaxsi-sortix.a: $(SORTIXOBJS)
	ar rcs libmaxsi-sortix.a $(SORTIXOBJS)

libc.a: libmaxsi.a
	ln -sf $< $@ 

libc.so: libmaxsi.so
	ln -sf $< $@ 

# libmaxsi

*.cpp: $(HEADERS)

%.o: %.cpp
	g++ -c $< -o $@ $(CXXFLAGS)
	
%.h: hsrc/%.h
	echo "/* WARNING: This header is generated - edits will be lost! */" > $@
	mxmpp -I decl $< >> $@

%.o: %.s
	as $(ASFLAGS) $< -o $@

%.o: %.asm
	nasm $(CPUNASMFLAGS) $< -o $@

# libc

c/*.c: $(CHEADERS)

c/%.o: c/%.c
	gcc -c $< -o $@ $(CFLAGS)

c/h/%.h: c/hsrc/%.h
	mkdir -p c/h
	mkdir -p c/h/sys
	echo "/* WARNING: This header is generated - edits will be lost! */" > $@
	mxmpp -I decl -I c/decl $< >> $@

# libmaxsi-sortix

sortix/*.cpp: $(HEADERS)

sortix/%.o: %.cpp $(HEADERS)
	g++ -c $< -o $@ $(SORTIXFLAGS)

sortix/%.o: sortix/%.cpp
	g++ -c $< -o $@ $(SORTIXFLAGS)

clean:
	rm -f *.o sortix/*.o c/*.o *.a *.so $(CHEADERS) $(HEADERS)

