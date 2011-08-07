ifndef CPU
    CPU=x86
endif

ifeq ($(CPU),x86)
	X86FAMILY=1
    CPUDEFINES=-DPLATFORM_X86
    CPUFLAGS=-m32
    CPULDFLAGS=-melf_i386
    CPUASFLAGS=-32
    CPUNASMFLAGS=-felf32
    CPUOBJS=$(CPU)/boot.o $(CPU)/base.o $(CPU)/x86.o
endif

ifeq ($(CPU),x64)
	X86FAMILY=1
    CPUDEFINES=-DPLATFORM_X64
    CPUFLAGS=-m64 -ffreestanding -mcmodel=large -mno-red-zone
    CPULDFLAGS=-melf64-little -z max-page-size=0x1000
    CPUASFLAGS=-64
    CPUNASMFLAGS=-felf64
    CPUOBJS=$(CPU)/base.o $(CPU)/x64.o
endif

ifdef X86FAMILY
    CPUOBJS:=$(CPUOBJS) \
             $(CPU)/memorymanagement-asm.o \
             $(CPU)/interrupt.o \
             $(CPU)/gdt.o
    CPUFLAGS:=$(CPUFLAGS) -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow
endif

DIRS=. x64 x86

DEFINES:=-DSORTIX_KERNEL $(CPUDEFINES)
DEFINES:=$(DEFINES) -DPLATFORM_VIRTUAL_MEMORY
DEFINES:=$(DEFINES) -DPLATFORM_KERNEL_HEAP
#DEFINES:=$(DEFINES) -DPLATFORM_SERIAL
#DEFINES:=$(DEFINES) -DPLATFORM_HTP
#DEFINES:=$(DEFINES) -DCONWAY
#DEFINES:=$(DEFINES) -DPONG
DEFINES:=$(DEFINES) -DINITRD
CPPFLAGSRELEASE=-s -O3
CPPFLAGSDEBUG=
CPPFLAGS=-I.. $(CPUDEFINES) $(CPUFLAGS) -std=gnu++0x -Wall -Wextra -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-rtti -fno-stack-protector $(DEFINES) $(CPPFLAGSRELEASE)
OBJS=$(CPUOBJS) kernel.o descriptor_tables.o isr.o time.o log.o iprintable.o panic.o  keyboard.o memorymanagement.o scheduler.o syscall.o application.o pong.o sound.o pci.o uart.o conway.o test.o http.o vgaterminal.o serialterminal.o descriptors.o ../libmaxsi/libmaxsi-sortix.a
JSOBJS:=$(subst .o,-js.o,$(OBJS))

all: sortix.bin

# jssortix compilation

jssortix: jssortix.bin

jssortix.bin: $(JSOBJS)
	ld -melf_i386 -Ttext 100000 -o jssortix.out $(JSOBJS)
	objcopy -O binary jssortix.out jssortix.bin

# x64 compilation
x64/boot.o: x64/boot.s
	as -32 $< -o $@

sortix-x64.tmp: $(OBJS) x64/boot.o
	ld -N -melf_x86_64 -Ttext 110000 -o sortix-x64-internal.out $(OBJS)
	objcopy -O binary sortix-x64-internal.out sortix-x64-internal.tmp
	objcopy --add-section      kernel64=sortix-x64-internal.tmp \
	        --set-section-flag kernel64=alloc,data,load,contents \
	        $(CPU)/boot.o
	ld -melf_i386 -Ttext 100000 $(CPU)/boot.o -o $@
	# move section to 0x11000
	objcopy --change-section-address kernel64=1114112 $@

# x86 compilation

sortix-x86.tmp: $(OBJS)
	ld -melf_i386 -Ttext 100000 -o $@ $(OBJS)

# general rules

sortix.bin: sortix-$(CPU).tmp
	cp -vu $< $@

%.o: %.cpp
	g++ -c $< -o $@ $(CPPFLAGS)

%.o: %.s
	as $(CPUASFLAGS) $< -o $@

%.o: %.asm
	nasm $(CPUNASMFLAGS) $< -o $@

%-js.o: %.cpp
	g++ -c $< -o $@ $(CPPFLAGS) -DJSSORTIX

%-js.o: %.s
	as $(CPUASFLAGS) $< -o $@

%-js.o: %.asm
	nasm $(CPUNASMFLAGS) $< -o $@

clean:
	for D in $(DIRS); do rm -fv $$D/*.o $$D/*.bin $$D/*.out $$D/*.tmp; done

# Local machine

install: sortix.bin
	cp sortix.bin /boot

uninstall:
	rm -f /boot/sortix.bin

# Remote machine

install-remote: sortix.bin
	scp -r ./ 192.168.2.6:/home/sortie/Desktop/sortix	
	scp sortix.bin root@192.168.2.6:/boot
	ssh root@192.168.2.6 "init 6"

uninstall-remote:
	ssh root@192.168.2.6 "rm /boot/sortix.bin"
