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

LIBMAXSIROOT=$(OSROOT)/libmaxsi

LIBC=$(LIBMAXSIROOT)/start.o $(LIBMAXSIROOT)/libc.a
LIBS=$(LIBC)

CPPFLAGS=$(CPUDEFINES)
FLAGS=-nostdinc -nostdlib -fno-builtin -nostartfiles -nodefaultlibs
INCLUDES=-I $(LIBMAXSIROOT)/c/h/ -I $(OSROOT)/

LD=ld
LDFLAGS=$(CPULDFLAGS) 
CC=gcc
CFLAGS=$(CPUFLAGS) $(FLAGS) $(INCLUDES)
CXX=g++
CXXFLAGS=$(CPUFLAGS) $(FLAGS) $(INCLUDES) -fno-exceptions


