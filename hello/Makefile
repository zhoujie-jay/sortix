# This is a bit bothersome design, but it should serve well enough as a simple
# cross-compiler for the Sortix operating system.

ifndef CPU
    CPU=x86
endif

ifeq ($(CPU),x86)
	X86FAMILY=1
    CPUDEFINES=-DPLATFORM_X86
    CPUFLAGS=-m32
    CPULDFLAGS=-melf_i386
endif

ifeq ($(CPU),x64)
	X86FAMILY=1
    CPUDEFINES=-DPLATFORM_X64
    CPUFLAGS=-m64
    CPULDFLAGS=-melf_x86_64
endif

LIBMAXSIROOT=../libmaxsi

INCLUDES=-I $(LIBMAXSIROOT)/c/h/ -I ../
CPPFLAGS=$(CPUDEFINES)

LD=ld
LDFLAGS=$(CPULDFLAGS) -Ttext 400000 $(LIBMAXSIROOT)/start.o
CC=gcc
CFLAGS=$(CPUFLAGS) -nostdinc -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-stack-protector $(INCLUDES)
CXX=g++
CXXFLAGS=$(CPUFLAGS) -nostdinc -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-rtti -fno-stack-protector $(INCLUDES)
LIBC=$(LIBMAXSIROOT)/libc.a

all: hello

hello: hello.o
	$(LD) $(LDFLAGS) hello.o -o hello.tmp $(LIBC)
	objcopy -O binary hello.tmp hello

hello.o: hello.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O2 -c hello.cpp -o hello.o

clean:
	rm -f hello.tmp hello.o hello

