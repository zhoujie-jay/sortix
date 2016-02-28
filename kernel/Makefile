SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

# Default values in case the user doesn't override these variables.
OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CPPFLAGS?=
CXXFLAGS?=$(OPTLEVEL)

# Base compiler options and definitions.

CPPFLAGS:=$(CPPFLAGS) -I. -Iinclude -D__is_sortix_kernel
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -ffreestanding -fbuiltin -std=gnu++11 \
                      -fno-exceptions -fno-rtti

ifeq ($(PANIC_SHORT),1)
    CPPFLAGS:=$(CPPFLAGS) -DPANIC_SHORT
endif
ifdef VERSION
    CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
endif

# Architecture-dependent options and definitions.

BOOTOBJS:=

ifeq ($(CPU),x86)
    X86FAMILY:=1
    CPUDIR:=$(CPU)
    CPUOBJS:=$(CPUDIR)/boot.o
endif

ifeq ($(CPU),x64)
    X86FAMILY:=1
    CPUDIR:=$(CPU)
    CXXFLAGS:=$(CXXFLAGS) -mno-red-zone -mno-mmx -mno-sse -mno-sse2
    CPUOBJS:=$(CPUDIR)/boot.o
endif

ifndef CPUDIR
    CPUDIR=.
endif

ifdef X86FAMILY
    CPUOBJS:=$(CPUOBJS) \
             $(CPUDIR)/memorymanagement.o \
             x86-family/memorymanagement.o \
             $(CPUDIR)/interrupt.o \
             x86-family/interrupt.o \
             x86-family/ioport.o \
             x86-family/pic.o \
             x86-family/gdt.o \
             x86-family/idt.o \
             $(CPUDIR)/syscall.o \
             x86-family/cmos.o \
             x86-family/time.o \
             x86-family/pat.o \
             x86-family/float.o \
             x86-family/ps2.o \
             x86-family/x86-family.o
endif

# Object files that constitute the kernel.

CRTI_OBJ=$(CPUDIR)/crti.o
CRTBEGIN_OBJ=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)
CRTN_OBJ=$(CPUDIR)/crtn.o

LIBS=\
-nostdlib \
-lk \
-lgcc \

OBJS=\
$(CPUOBJS) \
addralloc.o \
alarm.o \
clock.o \
com.o \
copy.o \
$(CPUDIR)/kthread.o \
descriptor.o \
disk/ahci/ahci.o \
disk/ahci/hba.o \
disk/ahci/port.o \
disk/ata/ata.o \
disk/ata/hba.o \
disk/ata/port.o \
disk/node.o \
dtable.o \
elf.o \
fcache.o \
fs/full.o \
fsfunc.o \
fs/kram.o \
fs/null.o \
fs/random.o \
fs/user.o \
fs/util.o \
fs/zero.o \
gpu/bga/bga.o \
hostname.o \
identity.o \
initrd.o \
inode.o \
interlock.o \
interrupt.o \
ioctx.o \
io.o \
kb/kblayout.o \
kb/ps2.o \
kernelinfo.o \
kernel.o \
kthread.o \
lfbtextbuffer.o \
libk.o \
linebuffer.o \
log.o \
logterminal.o \
memorymanagement.o \
mouse/ps2.o \
mtable.o \
net/fs.o \
op-new.o \
panic.o \
partition.o \
pci-mmio.o \
pci.o \
pipe.o \
poll.o \
process.o \
psctl.o \
ptable.o \
random.o \
refcount.o \
registers.o \
resource.o \
scheduler.o \
segment.o \
signal.o \
sockopt.o \
string.o \
syscall.o \
textbuffer.o \
textterminal.o \
thread.o \
time.o \
timer.o \
uart.o \
user-timer.o \
vga.o \
vgatextbuffer.o \
video.o \
vnode.o \
worker.o \

ALLOBJS=\
$(CRTI_OBJ) \
$(OBJS) \
$(CRTN_OBJ) \
end.o

LINK_OBJECTS=\
$(CRTI_OBJ) $(CRTBEGIN_OBJ) $(OBJS) $(LIBS) $(CRTN_OBJ) $(CRTEND_OBJ) end.o

# Rules and recipes for building the kernel.

all: kernel

kernel: sortix.bin

.PHONY: all kernel headers clean install install-include-dirs install-headers \
        install-kernel install-kernel-binary

headers:

# x64 compilation
ifeq ($(CPU),x64)

sortix-x86_64.bin: $(ALLOBJS)
	$(CXX) $(CXXFLAGS) -Wl,-Ttext -Wl,100000 -Wl,-z -Wl,max-page-size=0x1000 $(LINK_OBJECTS) -o $@

sortix.bin: sortix-x86_64.bin
	$(OBJCOPY) $< -O elf32-i386-sortix $@

endif

# x86 compilation
ifeq ($(CPU),x86)

sortix.bin: $(ALLOBJS)
	$(CXX) $(CXXFLAGS) -Wl,-Ttext -Wl,100000 $(LINK_OBJECTS) -o $@

endif

%: %.kblayout
	kblayout-compiler --format=sortix-kblayout-1 --compression=none $< -o $@

kb/default-kblayout.h: kb/default-kblayout
	carray -cgis --uint8_t --guard=SORTIX_KB_DEFAULT_KBLAYOUT_H --identifier=default_kblayout $< -o $@

vgafont.h: vgafont.f16
	carray -gis --uint8_t --guard=VGAFONT_H --identifier=vgafont $< -o $@

*.cpp */*.cpp */*/*.cpp: | kb/default-kblayout.h vgafont.h

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

%.o: %.S
	$(CXX) -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

clean:
	rm -f $(ALLOBJS) sortix.bin
	rm -f *.bin *.out *.tmp
	rm -f *.o */*.o */*/*.o
	rm -f kb/default-kblayout kb/default-kblayout.h
	rm -f vgafont.h

# Installation into sysroot
install: install-headers install-kernel

install-include-dirs: headers
	mkdir -p $(DESTDIR)$(INCLUDEDIR)

install-headers: install-include-dirs headers
	cp -RTv include $(DESTDIR)$(INCLUDEDIR)

install-kernel: install-kernel-binary

install-kernel-binary: sortix.bin
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp sortix.bin $(DESTDIR)$(BOOTDIR)/sortix.bin
