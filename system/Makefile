include ../platform.mak

# Protect the user from accidentally replacing his build operating system with a broken Sortix.
ifeq ($(HOST_IS_SORTIX),0)
$(error You are about to build Sortix as $(HOST) and possibly overwrite your root filesystem. Use the HOST=cpu-sortix environmental variable if you wish to cross-compile)
endif

SRCDIR:=..
BOOTDIR:=$(ROOT)/boot/$(cputype)
KERNEL:=$(BOOTDIR)/sortix.bin
INITRD_COMPRESSION:=.gz
INITRD:=$(BOOTDIR)/sortix.initrd
INITRD_FILTER:=$(BOOTDIR)/sortix.initrd.filter

all: $(KERNEL) $(INITRD)$(INITRD_COMPRESSION)

# Delete various temporary build files.
clean:
	$(MAKE) -B -C $(SRCDIR)/doc clean
	$(MAKE) -B -C $(SRCDIR)/libc clean
	$(MAKE) -B -C $(SRCDIR)/libm clean
	$(MAKE) -B -C $(SRCDIR)/libpthread clean
	$(MAKE) -B -C $(SRCDIR)/dispd clean
	$(MAKE) -B -C $(SRCDIR)/bench clean
	$(MAKE) -B -C $(SRCDIR)/games clean
	$(MAKE) -B -C $(SRCDIR)/mkinitrd clean
	$(MAKE) -B -C $(SRCDIR)/utils clean
	$(MAKE) -B -C $(SRCDIR)/ext clean
	$(MAKE) -B -C $(SRCDIR)/mbr clean
	$(MAKE) -B -C $(SRCDIR)/kernel clean

# Set up the base filesystem.
fsh:
	mkdir -p $(ROOT)/
	mkdir -p $(ROOT)/boot
	mkdir -p $(ROOT)/boot/$(cputype)
	mkdir -p $(ROOT)/dev
	mkdir -p $(ROOT)/etc
	mkdir -p $(ROOT)/etc/$(cputype)
	mkdir -p $(ROOT)/etc/skel
	mkdir -p $(ROOT)/home
	mkdir -p $(ROOT)/include
	mkdir -p $(ROOT)/include/$(cputype)
	mkdir -p $(ROOT)/mnt
	mkdir -p $(ROOT)/root
	mkdir -p $(ROOT)/share
	mkdir -p $(ROOT)/src
	mkdir -p $(ROOT)/tmp
	mkdir -p $(ROOT)/$(cputype)/bin
	mkdir -p $(ROOT)/$(cputype)/lib

# Install the system headers.
system-headers:
	$(MAKE) fsh ROOT=$(ROOT)
	$(MAKE) system-headers_internal ROOT=$(ROOT)

system-headers_internal:
	$(MAKE) -B -C $(SRCDIR)/kernel install-headers DESTDIR=$(ROOT)
	$(MAKE) -B -C $(SRCDIR)/libc install-headers DESTDIR=$(ROOT)
	$(MAKE) -B -C $(SRCDIR)/libm install-headers DESTDIR=$(ROOT)
	$(MAKE) -B -C $(SRCDIR)/libpthread install-headers DESTDIR=$(ROOT)

# Rebuild and install the entire system.
system:
	$(MAKE) fsh ROOT=$(ROOT)
	$(MAKE) system-headers_internal ROOT=$(ROOT)
	$(MAKE) -B -C $(SRCDIR)/doc install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/libc install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/libm install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/libpthread install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/dispd install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/games install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/mkinitrd install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/utils install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/bench install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/ext install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/mbr install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/lib
	$(MAKE) -B -C $(SRCDIR)/kernel install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/libs

# Rebuild and install the kernel
kernel:
	$(MAKE) -B -C $(SRCDIR)/kernel install-headers DESTDIR=$(ROOT)
	$(MAKE) -B -C $(SRCDIR)/kernel install DESTDIR=$(ROOT) C_INCLUDE_PATH=$(ROOT)/include CPLUS_INCLUDE_PATH=$(ROOT)/include LIBRARY_PATH=$(ROOT)/$(cputype)/libs

# Rebuild the initrd and the kernel.
boot-files: initrd kernel

# Regenerate the initrd from $ROOT.
initrd:
	mkdir -p $(BOOTDIR)
	rm -f $(INITRD_FILTER)
	echo exclude /boot >> $(INITRD_FILTER)
	echo exclude /dev >> $(INITRD_FILTER)
	echo exclude /next >> $(INITRD_FILTER)
	echo exclude /tmp >> $(INITRD_FILTER)
	echo exclude /$(OTHER_PLATFORM_1) >> $(INITRD_FILTER)
	echo exclude /etc/$(OTHER_PLATFORM_1) >> $(INITRD_FILTER)
	echo exclude /include/$(OTHER_PLATFORM_1) >> $(INITRD_FILTER)
	echo exclude /tix/$(OTHER_PLATFORM_1) >> $(INITRD_FILTER)
	mkinitrd $(ROOT)/ -o $(INITRD) -f $(INITRD_FILTER)
	rm -f $(INITRD_FILTER)
	gzip -v $(INITRD)

# Format a device with a tar filesystem.
device-format-tar:
	tar cvf DEVICE=$(DEVICE)

# Update the kernel on a device.
update-tar-device-kernel: $(KERNEL)
	cd $(BOOTDIR) ; head sortix.bin > /dev/null
	cd $(BOOTDIR) ; tar --delete --verbose --checkpoint=1 --blocking-factor=2048 --file=$(DEVICE) sortix.bin
	cd $(BOOTDIR) ; tar --append --verbose --checkpoint=1 --blocking-factor=2048 --file=$(DEVICE) sortix.bin

# Update the initrd on a device.
update-tar-device-initrd: $(INITRD)$(INITRD_COMPRESSION)
	cd $(BOOTDIR) ; head sortix.initrd$(INITRD_COMPRESSION) > /dev/null
	cd $(BOOTDIR) ; tar --delete --verbose --checkpoint=1 --blocking-factor=2048 --file=$(DEVICE) sortix.initrd$(INITRD_COMPRESSION)
	cd $(BOOTDIR) ; tar --append --verbose --checkpoint=1 --blocking-factor=2048 --file=$(DEVICE) sortix.initrd$(INITRD_COMPRESSION)

# Update the kernel and initrd on a device.
update-tar-device-image: $(KERNEL) $(INITRD)$(INITRD_COMPRESSION)
	cd $(BOOTDIR) ; head sortix.bin sortix.initrd$(INITRD_COMPRESSION) > /dev/null
	cd $(BOOTDIR) ; tar --create --verbose --checkpoint=1 --blocking-factor=2048 --file=$(DEVICE) sortix.bin sortix.initrd$(INITRD_COMPRESSION)

# Build an entire new Sortix system and install onto this device.
install-onto-device:
	$(MAKE) system ROOT=/next
	$(MAKE) initrd ROOT=/next
	$(MAKE) update-tar-device-image ROOT=/next DEVICE=$(DEVICE)

# Misc. stuff down here.
fetch-device-stuff: fetch-device-kernel fetch-device-initrd

fetch-device-kernel:
	cd $(BOOTDIR) ; tar xvf $(DEVICE) sortix.bin

fetch-device-initrd:
	cd $(BOOTDIR) ; tar xvf $(DEVICE)
#
