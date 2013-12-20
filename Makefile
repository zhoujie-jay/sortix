SOFTWARE_MEANT_FOR_SORTIX=1
MAKEFILE_NOT_MEANT_FOR_SORTIX=1
include compiler.mak
include version.mak

MODULES=doc libc libm dispd games mkinitrd mxmpp utils bench ext mbr kernel

ifndef SYSROOT
  SYSROOT:=$(shell pwd)/sysroot
  SUBMAKE_OPTIONS:=$(SUBMAKE_OPTIONS) "SYSROOT=$(SYSROOT)"
endif

ifndef SYSROOT_OVERLAY
  SYSROOT_OVERLAY:=$(shell pwd)/sysroot-overlay
  SUBMAKE_OPTIONS:=$(SUBMAKE_OPTIONS) "SYSROOT_OVERLAY=$(SYSROOT_OVERLAY)"
endif

SORTIX_BUILDS_DIR?=builds
SORTIX_PORTS_DIR?=ports
SORTIX_RELEASE_DIR?=release
SORTIX_REPOSITORY_DIR?=repository

include dirs.mak

BUILD_NAME:=sortix_$(VERSION)_$(MACHINE)
DEBNAME:=sortix_$(VERSION)_$(MACHINE)

INITRD:=$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).initrd

SUBMAKE_OPTIONS:=$(SUBMAKE_OPTIONS) VERSION=$(VERSION) PREFIX= EXEC_PREFIX=/$(HOST)

.PHONY: all
all: sysroot

.PHONY: build-tools
build-tools:
	$(MAKE) -C mkinitrd
	$(MAKE) -C mxmpp
	$(MAKE) -C tix

.PHONY: install-build-tools
install-build-tools:
	$(MAKE) -C mkinitrd install
	$(MAKE) -C mxmpp install
	$(MAKE) -C tix install

.PHONY: sysroot-fsh
sysroot-fsh:
	mkdir -p "$(SYSROOT)"
	for DIRNAME in boot etc include; do (\
	  mkdir -p "$(SYSROOT)/$$DIRNAME" &&\
	  mkdir -p "$(SYSROOT)/$$DIRNAME/$(HOST)" \
	) || exit $$?; done;
	mkdir -p "$(SYSROOT)/$(HOST)"
	for DIRNAME in bin lib; do (\
	  mkdir -p "$(SYSROOT)/$(HOST)/$$DIRNAME" \
	) || exit $$?; done;
	mkdir -p "$(SYSROOT)/etc/skel"
	mkdir -p "$(SYSROOT)/home"
	mkdir -p "$(SYSROOT)/mnt"
	mkdir -p "$(SYSROOT)/share"
	mkdir -p "$(SYSROOT)/src"
	mkdir -p "$(SYSROOT)/tmp"
	echo "root::0:0:root:/root:sh" > "$(SYSROOT)/etc/passwd"
	echo "root::0:root" > "$(SYSROOT)/etc/group"

.PHONY: sysroot-base-headers
sysroot-base-headers: sysroot-fsh
	(for D in libc libm kernel; do ($(MAKE) -C $$D install-headers $(SUBMAKE_OPTIONS) DESTDIR="$(SYSROOT)") || exit $$?; done)

.PHONY: sysroot-system
sysroot-system: sysroot-fsh sysroot-base-headers
	(for D in $(MODULES); do ($(MAKE) -C $$D $(SUBMAKE_OPTIONS) && $(MAKE) -C $$D install $(SUBMAKE_OPTIONS) DESTDIR="$(SYSROOT)") || exit $$?; done)

.PHONY: sysroot-source
sysroot-source: sysroot-fsh
	cp compiler.mak -t "$(SYSROOT)/src"
	cp dirs.mak -t "$(SYSROOT)/src"
	cp platform.mak -t "$(SYSROOT)/src"
	cp version.mak -t "$(SYSROOT)/src"
	cp COPYING-GPL -t "$(SYSROOT)/src"
	cp COPYING-LGPL -t "$(SYSROOT)/src"
	cp README -t "$(SYSROOT)/src"
	(for D in $(MODULES); do (cp -LR $$D -t "$(SYSROOT)/src" && $(MAKE) -C "$(SYSROOT)/src/$$D" clean) || exit $$?; done)
	cp -LR system -t "$(SYSROOT)/src"

.PHONY: sysroot-ports
sysroot-ports: sysroot-fsh sysroot-base-headers sysroot-system sysroot-source
	@SORTIX_PORTS_DIR="$(SORTIX_PORTS_DIR)" \
	 SORTIX_REPOSITORY_DIR="$(SORTIX_REPOSITORY_DIR)" \
	 SYSROOT="$(SYSROOT)" \
	 HOST="$(HOST)" \
	 MAKE="$(MAKE)" \
	 MAKEFLAGS="$(MAKEFLAGS) $(SUBMAKE_OPTIONS)" \
	 ./build-ports.sh

.PHONY: sysroot-overlay
sysroot-overlay: sysroot-fsh sysroot-system sysroot-ports
	! [ -d "$(SYSROOT_OVERLAY)" ] || \
	cp -R --preserve=mode,timestamp,links "$(SYSROOT_OVERLAY)" -T "$(SYSROOT)"

.PHONY: sysroot-user-skel
sysroot-user-skel: sysroot-fsh sysroot-system sysroot-ports sysroot-overlay
	cp "$(SYSROOT)/share/doc/welcome" -t "$(SYSROOT)/etc/skel"

.PHONY: sysroot-home-directory
sysroot-home-directory: sysroot-fsh sysroot-system sysroot-ports sysroot-overlay sysroot-user-skel
	mkdir -p "$(SYSROOT)/root"
	cp -R "$(SYSROOT)/etc/skel" -T "$(SYSROOT)/root"

.PHONY: sysroot
sysroot: sysroot-system sysroot-source sysroot-ports sysroot-overlay sysroot-home-directory

$(SORTIX_REPOSITORY_DIR):
	mkdir -p $@

$(SORTIX_REPOSITORY_DIR)/$(HOST): $(SORTIX_REPOSITORY_DIR)
	mkdir -p $@

.PHONY: clean-core
clean-core:
	(for D in $(MODULES) tix; do $(MAKE) clean $(SUBMAKE_OPTIONS) --directory $$D || exit $$?; done)

.PHONY: clean-ports
clean-ports:
	@SORTIX_PORTS_DIR="$(SORTIX_PORTS_DIR)" \
	 HOST="$(HOST)" \
	 MAKE="$(MAKE)" \
	 MAKEFLAGS="$(MAKEFLAGS) $(SUBMAKE_OPTIONS)" \
	 ./clean-ports.sh

.PHONY: clean-builds
clean-builds:
	rm -rf "$(SORTIX_BUILDS_DIR)"
	rm -f sortix.initrd
	rm -f sortix.iso
	rm -f sortix.iso.xz

.PHONY: clean-release
clean-release:
	rm -rf "$(SORTIX_RELEASE_DIR)"

.PHONY: clean-repository
clean-repository:
	rm -rf "$(SORTIX_REPOSITORY_DIR)"

.PHONY: clean-sysroot
clean-sysroot:
	rm -rf "$(SYSROOT)"

.PHONY: clean
clean: clean-core clean-ports

.PHONY: mostlyclean
mostlyclean: clean-core clean-ports clean-builds clean-release clean-sysroot

.PHONY: distclean
distclean: clean-core clean-ports clean-builds clean-release clean-repository clean-sysroot

.PHONY: most-things
most-things: sysroot initrd deb tar iso

.PHONY: everything
everything: most-things iso.xz

# Targets that build multiple architectures.

.PHONY: all-archs
all-archs:
	$(MAKE) clean
	$(MAKE) all HOST=i486-sortix
	$(MAKE) clean
	$(MAKE) all HOST=x86_64-sortix

.PHONY: most-things-all-archs
most-things-all-archs:
	$(MAKE) clean
	$(MAKE) most-things HOST=i486-sortix
	$(MAKE) clean
	$(MAKE) most-things HOST=x86_64-sortix

.PHONY: everything-all-archs
everything-all-archs:
	$(MAKE) clean
	$(MAKE) everything HOST=i486-sortix
	$(MAKE) clean
	$(MAKE) everything HOST=x86_64-sortix

.PHONY: release-all-archs
release-all-archs:
	$(MAKE) clean
	$(MAKE) release HOST=i486-sortix
	$(MAKE) clean
	$(MAKE) release HOST=x86_64-sortix

# Initial ramdisk

$(INITRD): sysroot
	mkdir -p `dirname $(INITRD)`
	echo -n > $(INITRD).filter
	echo "exclude /boot" >> $(INITRD).filter
	echo "exclude /dev" >> $(INITRD).filter
	echo "exclude /next" >> $(INITRD).filter
	echo "exclude /tmp" >> $(INITRD).filter
	for OTHER_PLATFORM in $(OTHER_PLATFORMS); do \
	  echo "exclude /$$OTHER_PLATFORM" >> $(INITRD).filter; \
	  echo "exclude /etc/$$OTHER_PLATFORM" >> $(INITRD).filter; \
	  echo "exclude /include/$$OTHER_PLATFORM" >> $(INITRD).filter; \
	  echo "exclude /tix/$$OTHER_PLATFORM" >> $(INITRD).filter; \
	done;
	if ! which mkinitrd; then echo You need to install mkinitrd; fi
	mkinitrd --filter $(INITRD).filter "$(SYSROOT)" -o $(INITRD)
	rm -f $(INITRD).filter

.PHONY: initrd
initrd: $(INITRD)

sortix.initrd: $(INITRD)
	cp $(INITRD) sortix.initrd

# Local machine

.PHONY: install
install: sysroot initrd
	cp "$(SYSROOT)/boot/$(HOST)/sortix.bin" /boot/sortix.initrd
	cp $(INITRD) /boot/sortix.initrd
	cp debsrc/etc/grub.d/42_sortix /etc/grub.d/42_sortix
	chmod +x /etc/grub.d/42_sortix
	update-grub

.PHONY: uninstall
uninstall:
	rm -f /boot/sortix.bin
	rm -f /boot/sortix.initrd
	rm -f /etc/grub.d/42_sortix

# Packaging

$(SORTIX_BUILDS_DIR):
	mkdir -p $(SORTIX_BUILDS_DIR)

$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).tar.xz: sysroot $(INITRD) $(SORTIX_BUILDS_DIR)
	rm -rf $(SORTIX_BUILDS_DIR)/tardir
	mkdir -p $(SORTIX_BUILDS_DIR)/tardir
	mkdir -p $(SORTIX_BUILDS_DIR)/tardir/boot
	cp "$(SYSROOT)/boot/$(HOST)/sortix.bin" $(SORTIX_BUILDS_DIR)/tardir/boot/sortix.bin
	cp $(INITRD) $(SORTIX_BUILDS_DIR)/tardir/boot/sortix.initrd
	cp -R debsrc -T $(SORTIX_BUILDS_DIR)/tardir
	rm -rf $(SORTIX_BUILDS_DIR)/tardir/DEBIAN
	tar --create --xz --file $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).tar.xz -C $(SORTIX_BUILDS_DIR)/tardir `ls $(SORTIX_BUILDS_DIR)/tardir`
	rm -rf $(SORTIX_BUILDS_DIR)/tardir

.PHONY: tar
tar: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).tar.xz

$(SORTIX_BUILDS_DIR)/$(DEBNAME).deb: sysroot $(INITRD) $(SORTIX_BUILDS_DIR)
	rm -rf $(SORTIX_BUILDS_DIR)/$(DEBNAME)
	mkdir -p $(SORTIX_BUILDS_DIR)/$(DEBNAME)
	mkdir -p $(SORTIX_BUILDS_DIR)/$(DEBNAME)/boot
	cp "$(SYSROOT)/boot/$(HOST)/sortix.bin" $(SORTIX_BUILDS_DIR)/$(DEBNAME)/boot/sortix.bin
	cp $(INITRD) $(SORTIX_BUILDS_DIR)/$(DEBNAME)/boot/sortix.initrd
	expr \( `stat --printf="%s" $(SORTIX_BUILDS_DIR)/$(DEBNAME)/boot/sortix.bin` \
	      + `stat --printf="%s" $(SORTIX_BUILDS_DIR)/$(DEBNAME)/boot/sortix.initrd` \
	      + 1023 \) / 1024 > $(SORTIX_BUILDS_DIR)/$(DEBNAME)/boot/deb.size
	cp -R debsrc -T $(SORTIX_BUILDS_DIR)/$(DEBNAME)
	SIZE=`cat $(SORTIX_BUILDS_DIR)/$(DEBNAME)/boot/deb.size`; \
	cat debsrc/DEBIAN/control | \
	sed "s/SORTIX_PACKAGE_NAME/sortix/g" | \
	sed "s/SORTIX_VERSION/$(VERSION)/g" | \
	sed "s/SORTIX_ARCH/all/g" | \
	sed "s/SORTIX_SIZE/$$SIZE/g" | \
	cat > $(SORTIX_BUILDS_DIR)/$(DEBNAME)/DEBIAN/control
	rm $(SORTIX_BUILDS_DIR)/$(DEBNAME)/boot/deb.size
	dpkg --build $(SORTIX_BUILDS_DIR)/$(DEBNAME) $(SORTIX_BUILDS_DIR)/$(DEBNAME).deb
	rm -rf $(SORTIX_BUILDS_DIR)/$(DEBNAME)/DEBIAN
	rm -rf $(SORTIX_BUILDS_DIR)/$(DEBNAME)

.PHONY: deb
deb: $(SORTIX_BUILDS_DIR)/$(DEBNAME).deb

# Bootable images

$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso: sysroot $(INITRD) $(SORTIX_BUILDS_DIR)
	rm -rf $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	mkdir -p $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	cp -R isosrc -T $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	cp "$(SYSROOT)/boot/$(HOST)/sortix.bin" $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/sortix.bin
	cp $(INITRD) $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/sortix.initrd
	grub-mkrescue -o $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	rm -rf $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso

$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso.xz: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso $(SORTIX_BUILDS_DIR)
	xz -c $< > $@

.PHONY: iso
iso: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso

.PHONY: iso.xz
iso.xz: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso.xz

sortix.iso: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso
	cp $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso $@

sortix.iso.xz: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso.xz
	cp $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso.xz $@

# Release

$(SORTIX_RELEASE_DIR):
	mkdir -p $@

$(SORTIX_RELEASE_DIR)/$(VERSION): $(SORTIX_RELEASE_DIR)
	mkdir -p $@

$(SORTIX_RELEASE_DIR)/$(VERSION)/builds: $(SORTIX_RELEASE_DIR)/$(VERSION)
	mkdir -p $@

$(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(BUILD_NAME).iso.xz: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso.xz $(SORTIX_RELEASE_DIR)/$(VERSION)/builds
	cp $< $@

.PHONY: release-iso.xz
release-iso.xz: $(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(BUILD_NAME).iso.xz

$(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(DEBNAME).deb: $(SORTIX_BUILDS_DIR)/$(DEBNAME).deb $(SORTIX_RELEASE_DIR)/$(VERSION)/builds
	cp $< $@

.PHONY: release-deb
release-deb: $(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(DEBNAME).deb

$(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(BUILD_NAME).tar.xz: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).tar.xz $(SORTIX_RELEASE_DIR)/$(VERSION)/builds
	cp $< $@

.PHONY: release-tar
release-tar: $(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(BUILD_NAME).tar.xz

.PHONY: release-builds
release-builds: release-iso.xz release-deb release-tar

$(SORTIX_RELEASE_DIR)/$(VERSION)/doc: $(SORTIX_RELEASE_DIR)/$(VERSION) doc doc/*
	cp -R doc -T $(SORTIX_RELEASE_DIR)/$(VERSION)/doc
	rm -f $(SORTIX_RELEASE_DIR)/$(VERSION)/doc/.gitignore
	rm -f $(SORTIX_RELEASE_DIR)/$(VERSION)/doc/Makefile

.PHONY: release-doc
release-doc: $(SORTIX_RELEASE_DIR)/$(VERSION)/doc

$(SORTIX_RELEASE_DIR)/$(VERSION)/README: README $(SORTIX_RELEASE_DIR)/$(VERSION)
	cp $< $@

.PHONY: release-readme
release-readme: $(SORTIX_RELEASE_DIR)/$(VERSION)/README

$(SORTIX_RELEASE_DIR)/$(VERSION)/repository:
	mkdir -p $@

$(SORTIX_RELEASE_DIR)/$(VERSION)/repository/$(HOST): sysroot $(SORTIX_REPOSITORY_DIR)/$(HOST) $(SORTIX_RELEASE_DIR)/$(VERSION)/repository
	cp -R $(SORTIX_REPOSITORY_DIR)/$(HOST) -T $@

.PHONY: release-repository
release-repository: $(SORTIX_RELEASE_DIR)/$(VERSION)/repository/$(HOST)

.PHONY: release-arch
release-arch: release-builds release-doc release-readme release-repository

.PHONY: release-shared
release-shared: release-doc release-readme

.PHONY: release
release: release-arch release-shared

# Virtualization
.PHONY: run-virtualbox
run-virtualbox: sortix.iso
	virtualbox --startvm sortix

.PHONY: run-virtualbox-debug
run-virtualbox-debug: sortix.iso
	virtualbox --debug --start-running --startvm sortix

# Statistics
.PHONY: linecount
linecount:
	wc -l `find | grep -E '\.h$$|\.h\+\+$$|\.c$$|\.cpp$$|\.c\+\+$$|\.s$$|\.S$$|\.asm$$|Makefile$$' | grep -v sysroot | grep -v sysroot-overlay | grep -v ports` | sort -n
