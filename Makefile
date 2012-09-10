include compiler.mak
include version.mak

ifneq ($(BUILD_LIBC),0)
  MODULES:=$(MODULES) libmaxsi
endif
ALLMODULES:=$(ALLMODULES) libmaxsi

ifneq ($(BUILD_GAMES),0)
  MODULES:=$(MODULES) games
endif
ALLMODULES:=$(ALLMODULES) games

ifneq ($(BUILD_MKINITRD),0)
  MODULES:=$(MODULES) mkinitrd
endif
ALLMODULES:=$(ALLMODULES) mkinitrd

ifneq ($(BUILD_UTILS),0)
  MODULES:=$(MODULES) utils
endif
ALLMODULES:=$(ALLMODULES) utils

ifneq ($(BUILD_BENCH),0)
  MODULES:=$(MODULES) bench
endif
ALLMODULES:=$(ALLMODULES) bench

ifneq ($(BUILD_KERNEL),0)
  MODULES:=$(MODULES) sortix
endif
ALLMODULES:=$(ALLMODULES) sortix

ifndef SYSROOT
  SYSROOT:=$(shell pwd)/sysroot
  MFLAGS:=$(MFLAGS) SYSROOT=$(SYSROOT)
endif

ifndef PREFIXNAME
  PREFIXNAME:=/
  MFLAGS:=$(MFLAGS) PREFIXNAME=$(PREFIXNAME)
endif

include dirs.mak

DEBNAME:=sortix_$(VERSION)_$(CPU)
DEBSRCNAME:=sortix_$(VERSION)
DEBDIR:=builds/$(DEBNAME)
DEBSRCDIR:=builds/$(DEBSRCNAME)-src
DEBFILE:=builds/$(DEBNAME).deb
PACKAGENAME:=sortix
ISODIR:=builds/$(DEBNAME)-iso
ISOFILE:=builds/$(DEBNAME).iso
INITRD=$(INSTALLBOOTDIR)/$(HOST)/sortix.initrd

MFLAGS:=$(MFLAGS) VERSION=$(VERSION)

all: $(INITRD)

.PHONY: all suball sysroot-base-headers sysroot-fsh clean distclean \
        everything everything-all-archs all-archs linecount install uninstall \
        deb debfile debsource iso run-virtualbox run-virtualbox-debug \
        clean-builds clean-sysroot

suball: sysroot-base-headers
	(for D in $(MODULES); do ($(MAKE) all $(MFLAGS) --directory $$D && $(MAKE) install $(MFLAGS) --directory $$D) || exit $$?; done)

sysroot-base-headers: sysroot-fsh
	(for D in libmaxsi sortix; do ($(MAKE) install-headers $(MFLAGS) --directory $$D) || exit $$?; done)

sysroot-fsh:
	mkdir -p "$(SYSROOT)"
	for DIRNAME in bin boot lib include; do (\
	  mkdir -p "$(SYSROOT)/$$DIRNAME" &&\
	  mkdir -p "$(SYSROOT)/$$DIRNAME/$(HOST)" \
	) || exit $$?; done;
	if [ ! -e "$(SYSROOT)/usr" ]; then ln -s . "$(SYSROOT)/usr"; fi

clean:
	rm -f "$(INITRD)"
	rm -f sortix/sortix.initrd # Backwards compatibility, not needed for newer builds.
	rm -f initrd/* # Backwards compatibility, not needed for newer builds.
	(for D in $(ALLMODULES); do $(MAKE) clean $(MFLAGS) --directory $$D || exit $$?; done)

clean-builds:
	rm -rf builds/
	rm -f sortix.iso

clean-sysroot:
	rm -rf "$(SYSROOT)"

distclean: clean clean-builds clean-sysroot

everything: all deb iso

everything-all-archs:
	$(MAKE) clean $(MFLAGS)
	$(MAKE) everything $(MFLAGS) HOST=i486-sortix
	$(MAKE) clean $(MFLAGS)
	$(MAKE) everything $(MFLAGS) HOST=x86_64-sortix

all-archs:
	$(MAKE) clean $(MFLAGS)
	$(MAKE) all $(MFLAGS) HOST=i486-sortix
	$(MAKE) clean $(MFLAGS)
	$(MAKE) all $(MFLAGS) HOST=x86_64-sortix

# Initializing RamDisk
$(INITRD): suball
	mkinitrd/mkinitrd $(SYSROOT)/bin/$(HOST) -o $(INITRD)

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

# Packaging

deb: debfile debsource

debfile: all
	rm -rf $(DEBDIR)
	mkdir -p $(DEBDIR)
	mkdir -p $(DEBDIR)/boot
	cp sortix/sortix.bin $(DEBDIR)/boot
	cp $(INITRD) $(DEBDIR)/boot
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
	(cd builds/$(DEBNAME) && tar cfz ../$(DEBNAME).tar.gz `ls`)
	rm -rf $(DEBDIR)

debsource: all
	rm -rf $(DEBSRCDIR)
	mkdir -p $(DEBSRCDIR)
	for D in `ls | grep -v builds | grep -v sysroot`; do cp -r $$D $(DEBSRCDIR); done
	(cd $(DEBSRCDIR) && make distclean SYSROOT=$(shell pwd)/$(DEBSRCDIR)/sysroot)
	rm -rf $(DEBSRCDIR)/sysroot
	(cd builds && tar cfz $(DEBSRCNAME)-src.tar.gz $(DEBSRCNAME)-src)
	rm -rf $(DEBSRCDIR)

# Bootable images

$(ISOFILE): all debsource
	rm -rf $(ISODIR)
	mkdir -p builds
	mkdir -p $(ISODIR)
	cp -r isosrc/. $(ISODIR)
	cp sortix/sortix.bin $(ISODIR)/boot
	cp $(INITRD) $(ISODIR)/boot/sortix.initrd
	cp builds/$(DEBSRCNAME)-src.tar.gz $(ISODIR)
	grub-mkrescue -o $(ISOFILE) $(ISODIR)
	rm -rf $(ISODIR)

iso: $(ISOFILE)

sortix.iso: iso
	cp $(ISOFILE) $@

# Virtualization
run-virtualbox: sortix.iso
	virtualbox --startvm sortix

run-virtualbox-debug: sortix.iso
	virtualbox --debug --start-running --startvm sortix

