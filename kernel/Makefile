SOFTWARE_MEANT_FOR_SORTIX=1
include ../compiler.mak
include ../version.mak
include ../dirs.mak

# Default values in case the user doesn't override these variables.
OPTLEVEL?=-g -O2
CALLTRACE?=0
DISKWRITE?=1
CPPFLAGS?=
CXXFLAGS?=$(OPTLEVEL)

# Base compiler options and definitions.

CPPFLAGS:=$(CPPFLAGS) -I. -Iinclude -D__is_sortix_kernel
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -ffreestanding -fbuiltin -std=gnu++11 \
                      -fno-exceptions -fno-rtti

ifeq ($(PANIC_SHORT),1)
    CPPFLAGS:=$(CPPFLAGS) -DPANIC_SHORT
endif
CPPFLAGS:=$(CPPFLAGS) -DENABLE_DISKWRITE=$(DISKWRITE)
CPPFLAGS:=$(CPPFLAGS) -DENABLE_CALLTRACE=$(CALLTRACE)
ifdef VERSION
    CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
endif

# Architecture-dependent options and definitions.

BOOTOBJS:=

ifeq ($(CPU),x86)
    X86FAMILY:=1
    CPUOBJS:=$(CPU)/boot.o $(CPU)/base.o $(CPU)/x86.o
endif

ifeq ($(CPU),x64)
    X86FAMILY:=1
    CXXFLAGS:=$(CXXFLAGS) -mno-red-zone
    CPUOBJS:=$(CPU)/boot.o $(CPU)/base.o $(CPU)/x64.o
endif

ifdef X86FAMILY
    CPUOBJS:=$(CPUOBJS) \
             $(CPU)/memorymanagement.o \
             x86-family/memorymanagement.o \
             $(CPU)/interrupt.o \
             x86-family/interrupt.o \
             x86-family/pic.o \
             x86-family/gdt.o \
             x86-family/idt.o \
             $(CPU)/syscall.o \
             $(CPU)/thread.o \
             $(CPU)/process.o \
             x86-family/cmos.o \
             x86-family/time.o \
             x86-family/mtrr.o \
             x86-family/pat.o \
             x86-family/float.o \
             x86-family/x86-family.o
    # TODO: Are these -m flags even needed in the first place?
    CXXFLAGS:=$(CXXFLAGS) -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow
endif

# Object files that constitute the kernel.

CRTI_OBJ:=$(CPU)/crti.o
CRTBEGIN_OBJ:=$(shell $(HOSTCXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(HOSTCXX) $(CXXFLAGS) -print-file-name=crtend.o)
CRTN_OBJ:=$(CPU)/crtn.o

LIBS=\
-nostdlib \
-lk \
-lgcc \

OBJS=\
$(CPUOBJS) \
addralloc.o \
alarm.o \
ata.o \
bga.o \
calltrace.o \
clock.o \
com.o \
copy.o \
$(CPU)/calltrace.o \
$(CPU)/kthread.o \
crc32.o \
debugger.o \
descriptor.o \
dispmsg.o \
dtable.o \
elf.o \
fcache.o \
fsfunc.o \
fs/full.o \
fs/kram.o \
fs/null.o \
fs/user.o \
fs/util.o \
fs/zero.o \
identity.o \
initrd.o \
inode.o \
interlock.o \
interrupt.o \
ioctx.o \
io.o \
kb/layout/us.o \
kb/ps2.o \
kernelinfo.o \
kernel.o \
kthread.o \
lfbtextbuffer.o \
linebuffer.o \
log.o \
logterminal.o \
memorymanagement.o \
mtable.o \
net/fs.o \
op-new.o \
panic.o \
partition.o \
pci.o \
pipe.o \
poll.o \
process.o \
refcount.o \
resource.o \
scheduler.o \
segment.o \
signal.o \
string.o \
symbol.o \
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
	$(HOSTCXX) $(CXXFLAGS) -Wl,-Ttext -Wl,100000 -Wl,-z -Wl,max-page-size=0x1000 $(LINK_OBJECTS) -o $@

sortix.bin: sortix-x86_64.bin
	$(HOSTOBJCOPY) $< -O elf32-i386-sortix $@

endif

# x86 compilation
ifeq ($(CPU),x86)

sortix.bin: $(ALLOBJS)
	$(HOSTCXX) $(CXXFLAGS) -Wl,-Ttext -Wl,100000 $(LINK_OBJECTS) -o $@

endif

%.o: %.cpp
	$(HOSTCXX) -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

%.o: %.S
	$(HOSTCXX) -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

clean:
	rm -f $(ALLOBJS) sortix.bin
	rm -f $(wildcard *.bin) $(wildcard *.out) $(wildcard *.tmp)
	rm -f $(wildcard *.o) $(wildcard */*.o) $(wildcard */*/*.o)

# Installation into sysroot
install: install-headers install-kernel

install-include-dirs: headers
	mkdir -p $(DESTDIR)$(INCLUDEDIR)

install-headers: install-include-dirs headers
	cp -RTv include $(DESTDIR)$(INCLUDEDIR)

install-kernel: install-kernel-binary

install-kernel-binary: sortix.bin
	mkdir -p $(DESTDIR)$(BOOTDIR)/$(HOST)
	cp sortix.bin $(DESTDIR)$(BOOTDIR)/$(HOST)

