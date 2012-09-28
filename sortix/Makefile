include ../compiler.mak
include ../version.mak
include ../dirs.mak

ifndef OPTLEVEL
    OPTLEVEL:=-O2
endif

ifeq ($(CPU),x86)
    BUILDID=x86
    X86FAMILY=1
    CPUFLAGS=
    CPULDFLAGS=
    CPUNASMFLAGS=-felf32
    CPUOBJS:=$(CPU)/boot.o $(CPU)/base.o $(CPU)/x86.o
endif

ifeq ($(CPU),x64)
    BUILDID=x64
    X86FAMILY=1
    CPUFLAGS=-ffreestanding -mno-red-zone
    CPULDFLAGS=
    CPUNASMFLAGS=-felf64
    CPUOBJS:=$(CPU)/base.o $(CPU)/x64.o
endif

ifdef X86FAMILY
    CPUOBJS:=$(CPUOBJS) \
             $(CPU)/memorymanagement.o \
             x86-family/memorymanagement.o \
             $(CPU)/interrupt.o \
             $(CPU)/gdt.o \
             x86-family/gdt.o \
             $(CPU)/idt.o \
             x86-family/idt.o \
             $(CPU)/syscall.o \
             $(CPU)/thread.o \
             $(CPU)/scheduler.o \
             $(CPU)/process.o \
             x86-family/msr.o \
             x86-family/float.o \
             x86-family/x86-family.o
    CPUFLAGS:=$(CPUFLAGS) -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow
endif

DIRS=. x64 x86 x86-family fs kb kb/layout

DEFINES:=-DSORTIX_KERNEL -U_GNU_SOURCE
ifeq ($(PANIC_SHORT),1)
    DEFINES:=$(DEFINES) -DPANIC_SHORT
endif
ifeq ($(DISKWRITE),1)
    DEFINES:=$(DEFINES) -DENABLE_DISKWRITE=1
else
    DEFINES:=$(DEFINES) -DENABLE_DISKWRITE=0
endif
ifeq ($(CALLTRACE),1)
    DEFINES:=$(DEFINES) -DENABLE_CALLTRACE=1
else
    DEFINES:=$(DEFINES) -DENABLE_CALLTRACE=0
endif
ifdef VERSION
    DEFINES:=$(DEFINES) -DVERSIONSTR=\"$(VERSION)\"
endif

INCLUDES=-I. -Iinclude
CPPFLAGS=$(INCLUDES) $(DEFINES)
ifeq ($(DEBUG_KERNEL),1)
    FLAGSDEBUG=-g3
else
    FLAGSDEBUG=$(OPTLEVEL)
endif
FLAGS=$(CPUFLAGS) -Wall  -Wall -Wextra -nostdlib -nostartfiles \
      -nodefaultlibs -fno-stack-protector $(FLAGSDEBUG)
CFLAGS=$(FLAGS)
CXXFLAGS=$(FLAGS) -std=gnu++0x -fno-exceptions -fno-rtti
ASFLAGS=
NASMFLAGS=$(CPUNASMFLAGS)

STATICLIBS=\
-lc-sortix \
-lgcc \

HEADERDIRS:=$(shell find include -type d)
HEADERS:=$(shell find include -type f)

OBJS=$(CPUOBJS) \
kernel.o \
crc32.o \
string.o \
interrupt.o \
worker.o \
time.o \
log.o \
utf8.o \
memorymanagement.o \
calltrace.o \
$(CPU)/calltrace.o \
kthread.o \
$(CPU)/kthread.o \
interlock.o \
$(CPU)/interlock.o \
panic.o \
keyboard.o \
kb/ps2.o \
kb/layout/us.o \
scheduler.o \
syscall.o \
sound.o \
pci.o \
com.o \
uart.o \
terminal.o \
linebuffer.o \
logterminal.o \
textterminal.o \
serialterminal.o \
textbuffer.o \
vgatextbuffer.o \
lfbtextbuffer.o \
descriptors.o \
device.o \
refcount.o \
video.o \
bga.o \
vga.o \
kernelinfo.o \
elf.o \
process.o \
initrd.o \
thread.o \
io.o \
pipe.o \
filesystem.o \
directory.o \
mount.o \
signal.o \
fs/util.o \
fs/devfs.o \
fs/initfs.o \
fs/ramfs.o \
fs/videofs.o \
ata.o \

ALLOBJS=\
$(OBJS) \
end.o

all: sortix.bin

.PHONY: all headers clean install install-include-dirs install-headers

headers:

# x64 compilation
x64/boot.o: x64/boot.s
	$(HOSTAS) -64 $< -o $@

sortix-x64.tmp: $(ALLOBJS) x64/boot.o
	$(HOSTCXX) -nodefaultlibs -nostartfiles -Wl,-N -Wl,-melf_x86_64_sortix -Wl,-Ttext -Wl,100000 -Wl,-z -Wl,max-page-size=0x1000 -o sortix-x64-internal.out x64/boot.o $(OBJS) $(STATICLIBS) end.o
	$(HOSTOBJCOPY) sortix-x64-internal.out -O elf32-i386 sortix-x64.tmp

# x86 compilation

sortix-x86.tmp: $(ALLOBJS)
	$(HOSTCXX) -nodefaultlibs -nostartfiles -Wl,-melf_i386_sortix -Wl,-Ttext -Wl,100000 -o $@ $(OBJS) $(STATICLIBS) end.o

# general rules

sortix.bin: sortix-$(BUILDID).tmp
	cp -vu $< $@

%.o: %.cpp
	$(HOSTCXX) -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

%.o: %.s
	$(HOSTAS) $< -o $@ $(ASFLAGS)

%.o: %.asm
	nasm $(NASMFLAGS) $< -o $@

clean:
	for D in $(DIRS); do rm -f $$D/*.o $$D/*.bin $$D/*.out $$D/*.tmp; done

# Installation into sysroot
install: install-headers install-kernel

install-include-dirs: headers
	mkdir -p $(INSTALLINCLUDEDIR)
	DIRS=$$(echo $(HEADERDIRS) | \
	        tr ' ' '\n' | \
	        sed 's/include\/*//'); \
	for D in $$DIRS; do \
	  mkdir -p $(INSTALLINCLUDEDIR)/$$D || exit $?; \
	done

install-headers: install-include-dirs headers
	for SRC in $(HEADERS); do \
	  DEST=`echo $$SRC | sed 's/include\///'`; \
	  cp -P $$SRC $(INSTALLINCLUDEDIR)/$$DEST || exit $?; \
	done

install-kernel: install-kernel-binary

install-kernel-binary:
	mkdir -p $(INSTALLBOOTDIR)/$(HOST)
	cp -P sortix.bin $(INSTALLBOOTDIR)/$(HOST)

