ifndef O
    O:=-O3
endif

ifndef CPU
    CPU=x86
endif

ifeq ($(CPU),x86)
    BUILDID=x86
    X86FAMILY=1
    CPUDEFINES=-DPLATFORM_X86
    CPUFLAGS=-m32
    CPULDFLAGS=-melf_i386
    CPUASFLAGS=-32
    CPUNASMFLAGS=-felf32
    CPUOBJS=$(CPU)/boot.o $(CPU)/base.o $(CPU)/x86.o
endif

ifeq ($(CPU),x64)
    BUILDID=x64
    X86FAMILY=1
    CPUDEFINES=-DPLATFORM_X64
    CPUFLAGS=-m64 -ffreestanding -mno-red-zone
    CPULDFLAGS=-melf64-little -z max-page-size=0x1000
    CPUASFLAGS=-64
    CPUNASMFLAGS=-felf64
    CPUOBJS=$(CPU)/base.o $(CPU)/x64.o
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
             x86-family/x86-family.o
    CPUFLAGS:=$(CPUFLAGS) -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow
endif

DIRS=. x64 x86 x86-family fs kb kb/layout

DEFINES:=-DSORTIX_KERNEL -U_GNU_SOURCE $(CPUDEFINES)
ifeq ($(JSSORTIX),1)
    DEFINES:=$(DEFINES) -DPLATFORM_SERIAL -DJSSORTIX
    BUILDID:=$(BUILDID)-js
endif
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

INCLUDES=-I. -Iinclude -I../libmaxsi/preproc
CPPFLAGS=$(INCLUDES) $(DEFINES)
FLAGSRELEASE=-s $(O)
FLAGSDEBUG=
FLAGS=$(CPUFLAGS) -Wall  -Wall -Wextra -nostdlib -fno-builtin -nostartfiles \
      -nodefaultlibs -fno-stack-protector $(FLAGSRELEASE)
CFLAGS=$(FLAGS)
CXXFLAGS=$(FLAGS) -std=gnu++0x -fno-exceptions -fno-rtti
ASFLAGS=$(CPUASFLAGS)
NASMFLAGS=$(CPUNASMFLAGS)

STATICLIBS=\
../libmaxsi/libmaxsi-sortix.a \

HEADERDIRS:=$(shell find include -type d)
HEADERS:=$(shell find include -type f)

OBJS=$(CPUOBJS) \
kernel.o \
interrupt.o \
time.o \
log.o \
utf8.o \
memorymanagement.o \
calltrace.o \
$(CPU)/calltrace.o \
kthread.o \
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
event.o \
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
$(STATICLIBS) \
end.o # Must be last, determines kernel size.

JSOBJS:=$(subst .o,-js.o,$(OBJS))

all: sortix.bin

# jssortix compilation

jssortix: jssortix.bin

sortix-x86-js.tmp: $(OBJS)
	ld -melf_i386 -Ttext 100000 -o sortix-x86-js-internal.out $(OBJS)
	objcopy -O binary sortix-x86-js-internal.out $@

# x64 compilation
x64/boot.o: x64/boot.s
	as -64 $< -o $@

sortix-x64.tmp: $(OBJS) x64/boot.o
	ld -N -melf_x86_64 -Ttext 100000 -o sortix-x64-internal.out x64/boot.o $(OBJS)
	objcopy sortix-x64-internal.out -O elf32-i386 sortix-x64.tmp

# x86 compilation

sortix-x86.tmp: $(OBJS)
	ld -melf_i386 -Ttext 100000 -o $@ $(OBJS)

# general rules

sortix.bin: sortix-$(BUILDID).tmp
	cp -vu $< $@

%.o: %.cpp
	g++ -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

%.o: %.s
	as $(ASFLAGS) $< -o $@

%.o: %.asm
	nasm $(NASMFLAGS) $< -o $@

clean:
	for D in $(DIRS); do rm -f $$D/*.o $$D/*.bin $$D/*.out $$D/*.tmp; done

# Installation into sysroot
install:
	for DIR in $(HEADERDIRS); do \
		mkdir -p $(SYSROOT)/usr/$$DIR; \
	done
	for FILE in $(HEADERS); do \
		cp $$FILE $(SYSROOT)/usr/$$FILE; \
	done

