BITS:=$(shell getconf LONG_BIT)
ifndef CPU
    ifeq ($(BITS),64)
        CPU:=x64
    else
        CPU:=x86
    endif
    MFLAGS:=$(MFLAGS) CPU=$(CPU)
endif

ifndef O
    O=-O2
    MFLAGS:=$(MFLAGS) 0=$(O)
endif

ifeq ($(BENCH),1)
    EXTRAMODULES:=$(EXTRAMODULES) bench
endif

ifndef SYSROOT
    SYSROOT:=$(shell pwd)/sysroot
    MFLAGS:=$(MFLAGS) SYSROOT=$(SYSROOT)
endif

REMOTE=192.168.2.6
REMOTEUSER=sortie
REMOTECOPYDIR:=/home/$(REMOTEUSER)/Desktop/MaxsiOS
MODULES=libmaxsi games mkinitrd utils $(EXTRAMODULES) sortix
ALLMODULES=libmaxsi games mkinitrd utils bench sortix

VERSION=0.5
DEBNAME:=sortix_$(VERSION)_$(CPU)
DEBSRCNAME:=sortix_$(VERSION)
DEBDIR:=builds/$(DEBNAME)
DEBSRCDIR:=builds/$(DEBSRCNAME)-src
DEBFILE:=builds/$(DEBNAME).deb
PACKAGENAME:=sortix
ISODIR:=builds/$(DEBNAME)-iso
ISOFILE:=builds/$(DEBNAME).iso
INITRDDIR:=initrd
INITRD=sortix/sortix.initrd

all: $(INITRD)

suball:
	(for D in $(MODULES); do ($(MAKE) all $(MFLAGS) --directory $$D && $(MAKE) install $(MFLAGS) --directory $$D) || exit $?; done)

clean:
	rm -rf $(SYSROOT)
	rm -f $(INITRD)
	rm -f initrd/*
	(for D in $(ALLMODULES); do $(MAKE) clean $(MFLAGS) --directory $$D || exit $?; done)

distclean: clean cleanbuilds

cleanbuilds:
	rm -rf builds/
	rm -f sortix.iso

everything: all deb iso

everything-all-archs:
	$(MAKE) clean $(MFLAGS)
	$(MAKE) everything $(MFLAGS) CPU=x86
	$(MAKE) clean $(MFLAGS)
	$(MAKE) everything $(MFLAGS) CPU=x64

# Initializing RamDisk
$(INITRD): suball
	(cd $(INITRDDIR) && ../mkinitrd/mkinitrd * -o ../$(INITRD))

# Statistics
linecount:
	wc -l `find | grep -E '\.h$$|\.c$$|\.cpp$$|\.s$$|\.asm$$|Makefile$$' | grep -v sysroot` | sort -n

# Local machine

install: all
	cp sortix/sortix.bin /boot
	cp $(INITRD) /boot
	cp debsrc/etc/grub.d/42_sortix /etc/grub.d/42_sortix
	chmod +x /etc/grub.d/42_sortix
	update-grub

uninstall:
	rm -f /boot/sortix.bin
	rm -f /etc/grub.d/42_sortix
	update-grub

# Remote machine

install-remote: all
	scp -r ./ $(REMOTE):$(REMOTECOPYDIR)
	scp sortix/sortix.bin root@$(REMOTE):/boot
	scp $(INITRD) root@$(REMOTE):/boot
	ssh root@$(REMOTE) "init 6"

uninstall-remote:
	ssh root@$(REMOTE) "rm /boot/sortix.bin"

# Packaging

deb: debfile debsource

debfile: all
	rm -rf $(DEBDIR)
	mkdir -p $(DEBDIR)
	mkdir -p $(DEBDIR)/boot
	cp sortix/sortix.bin $(DEBDIR)/boot
	cp sortix/sortix.initrd $(DEBDIR)/boot
	expr \( `stat --printf="%s" $(DEBDIR)/boot/sortix.bin` \
	      + `stat --printf="%s" $(DEBDIR)/boot/sortix.initrd` \
	      + 1023 \) / 1024 > $(DEBDIR)/boot/deb.size
	cp -r debsrc/. $(DEBDIR)
	mkdir -p  $(DEBDIR)/boot
	SIZE=`cat $(DEBDIR)/boot/deb.size`; \
	cat debsrc/DEBIAN/control | \
	sed "s/SORTIX_PACKAGE_NAME/$(PACKAGENAME)/g" | \
	sed "s/SORTIX_VERSION/$(VERSION)/g" | \
	sed "s/SORTIX_ARCH/all/g" | \
	sed "s/SORTIX_SIZE/$$SIZE/g" | \
	cat > $(DEBDIR)/DEBIAN/control
	rm $(DEBDIR)/boot/deb.size
	dpkg --build $(DEBDIR) $(DEBFILE)
	rm -rf $(DEBDIR)/DEBIAN
	(cd builds/$(DEBNAME) && tar cfzv ../$(DEBNAME).tar.gz `ls`)
	rm -rf $(DEBDIR)

debsource: all
	rm -rf $(DEBSRCDIR)
	mkdir -p $(DEBSRCDIR)
	for D in `ls | grep -v builds | grep -v sysroot`; do cp -r $$D $(DEBSRCDIR); done
	(cd $(DEBSRCDIR) && make distclean)
	rm -rf $(DEBSRCDIR)/sysroot
	(cd builds && tar cfzv $(DEBSRCNAME)-src.tar.gz $(DEBSRCNAME)-src)
	rm -rf $(DEBSRCDIR)

# Bootable images

iso: all debsource
	rm -rf $(ISODIR)
	mkdir -p builds
	mkdir -p $(ISODIR)
	cp -r isosrc/. $(ISODIR)
	cp sortix/sortix.bin $(ISODIR)/boot
	cp $(INITRD) $(ISODIR)/boot/sortix.initrd
	cp builds/$(DEBSRCNAME)-src.tar.gz $(ISODIR) 
	grub-mkrescue -o $(ISOFILE) $(ISODIR)
	rm -rf $(ISODIR)

sortix.iso: iso
	cp $(ISOFILE) sortix.iso

# Virtualization
run-virtualbox: sortix.iso
	virtualbox --startvm sortix

run-virtualbox-debug: sortix.iso
	virtualbox --debug --start-running --startvm sortix


