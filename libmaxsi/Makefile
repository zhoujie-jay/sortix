ifndef CPU
    CPU=x86
endif

BINS=libc.a libg.a libmaxsi.a libmaxsi-sortix.a

ifeq ($(CPU),x86)
    CPUDEFINES=-DPLATFORM_X86
    CPUFLAGS=-m32
    CPULDFLAGS=-melf_i386
    CPUASFLAGS=-32
endif

ifeq ($(CPU),x64)
    CPU=x64
    CPUDEFINES=-DPLATFORM_X64
    CPUFLAGS=-fPIC -m64
    CPULDFLAGS=-melf_x86_64
    CPUASFLAGS=-64
endif

CPPINCLUDES=-I preproc -I ../sortix/include
CPPFLAGS=-DLIBMAXSI_LIBRARY -DSORTIX -U_GNU_SOURCE $(CPUDEFINES) $(CPPINCLUDES)
FLAGS=$(CPUFLAGS) -Wall -Wextra -nostdlib -fno-builtin -nostartfiles \
                  -nodefaultlibs -fno-stack-protector -nostdinc
CFLAGS=$(FLAGS) -std=c99
CXXFLAGS=$(FLAGS) -std=gnu++0x -fno-exceptions -nostdinc++ -fno-rtti
LDFLAGS=$(CPULDFLAGS)
ASFLAGS=$(CPUASFLAGS)

OBJS=\
ctype.o \
file.o \
fdio.o \
fpipe.o \
stdio.o \
dir.o \
fddir-sortix.o \
setjmp.o \
sortix-sound.o \
process.o \
thread.o \
io.o \
ioleast.o \
terminal.o \
kernelinfo.o \
init.o \
exit.o \
signal.o \
$(CPU)/signal.o \
$(CPU)/fork.o \
start.o \
time.o \
random.o \
abs.o \
env.o \
integer.o \
c++.o \
memory.o \
heap.o \
sort.o \
string.o \
error.o \
format.o \

UNPROCHEADERDIRS:=$(shell find include -type d)
UNPROCHEADERS:=$(shell find include -type f)
HEADERDIRS:=$(patsubst include%,preproc%,$(UNPROCHEADERDIRS))
HEADERS:=$(patsubst include%,preproc%,$(UNPROCHEADERS))

SORTIXOBJS=\
c++.o \
memory.o \
heap.o \
string.o \
error.o \
format.o \

SORTIXOBJS:=$(addprefix sortix/,$(SORTIXOBJS))
SORTIXCPPFLAGS:=-DSORTIX_KERNEL

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

libg.a: libc.a
	ln -sf $< $@

libg.so: libc.so
	ln -sf $< $@

start.o: $(CPU)/start.o
	ln -sf $< $@

# header preprocessing
$(HEADERDIRS):
	mkdir -p $@

preproc/%: include/% $(HEADERDIRS)
	mxmpp -I decl $< -o $@

headers: $(HEADERDIRS) $(HEADERS)

# standard library
%.o: %.c headers
	gcc -c $< -o $@ $(CPPFLAGS) $(CFLAGS)

%.o: %.cpp headers
	g++ -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

%.o: %.s
	as $(ASFLAGS) $< -o $@

# libmaxsi-sortix
sortix:
	mkdir -p sortix

sortix/%.o: %.cpp $(HEADERS) sortix
	g++ -c $< -o $@ $(CPPFLAGS) $(SORTIXCPPFLAGS) $(CXXFLAGS)

clean:
	rm -f *.o sortix/*.o c/*.o x86/*.o x64/*.o *.a *.so
	rm -f $(OBJS)
	rm -rf sortix $(HEADERDIRS)

# Installation into sysroot
install:
	mkdir -p $(SYSROOT)/usr/lib
	for F in $(BINS); do cp -P $$F $(SYSROOT)/usr/lib || exit $?; done
	for D in $(UNPROCHEADERDIRS); do mkdir -p $(SYSROOT)/usr/$$D || exit $?; done
	for SRC in $(HEADERS); do DEST=`echo $$SRC | sed 's/preproc/include/'`; cp $$SRC $(SYSROOT)/usr/$$DEST || exit $?; done
	mkdir -p $(SYSROOT)/usr/include
	cp start.o $(SYSROOT)/usr/lib/crtbegin.o
	touch deleteme.cpp
	g++ $(CPUFLAGS) -c deleteme.cpp -o deleteme.o
	for F in crt0.o crtn.o crt1.o crtend.o crtbeginT.o crti.o; do cp deleteme.o $(SYSROOT)/usr/lib/$$F; done
	for F in libgcc.so libm.so libstdc++.so; do ld $(CPULDFLAGS) -shared deleteme.o -o $(SYSROOT)/usr/lib/$$F; done
	rm -f deleteme.o deleteme.cpp

