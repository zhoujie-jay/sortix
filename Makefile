SOFTWARE_MEANT_FOR_SORTIX=1
include build-aux/platform.mak
include build-aux/compiler.mak
include build-aux/version.mak

MODULES=\
doc \
libc \
libm \
libpthread \
dispd \
bench \
carray \
editor \
ext \
games \
init \
mbr \
mkinitrd \
regress \
sh \
tix \
trianglix \
utils \
kernel

ifndef SYSROOT
  SYSROOT:=$(shell pwd)/sysroot
endif

ifndef SYSROOT_OVERLAY
  SYSROOT_OVERLAY:=$(shell pwd)/sysroot-overlay
endif

SORTIX_BUILDS_DIR?=builds
SORTIX_PORTS_DIR?=ports
SORTIX_RELEASE_DIR?=release
SORTIX_REPOSITORY_DIR?=repository

include build-aux/dirs.mak

BUILD_NAME:=sortix_$(VERSION)_$(MACHINE)

INITRD:=$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).initrd

.PHONY: all
all: sysroot

.PHONY: install
install: sysroot
	@if test -z '$(INSTALL_ROOTFS)' ; then \
	  echo "error: You must set INSTALL_ROOTFS to where you want Sortix installed" >&2; \
	  exit 1; \
	fi
	@if test -d '$(INSTALL_ROOTFS)' && test -z '$(STUPIDLY_FORCE_SORTIX_INSTALL_OVERWRITE)'; then \
	  for ENTRY in $$(ls -A "$(SYSROOT)"); do \
	    if test -e "$(INSTALL_ROOTFS)/$$ENTRY"; then \
	      echo "Error: Refusing to corrupt the existing installation at $(INSTALL_ROOTFS)" >&2; \
	      echo "Use sysmerge to update an existence installation." >&2; \
	      exit 1; \
	    fi; \
	  done; \
	fi
	cp -RTv "$(SYSROOT)" "$(INSTALL_ROOTFS)"
	@if test -n '$(INSTALL_ROOTFS_UUID)'; then \
	   echo '$(INSTALL_ROOTFS_UUID)' > "$(INSTALL_ROOTFS)/etc/rootfs.uuid"; \
	   SYSROOT="$(SYSROOT)" $(MAKE) create-install-rootfs-initrd; \
	else \
	   echo "Warning: INSTALL_ROOTFS_UUID was not set"; \
	   echo "Therefore: /etc/rootfs.uuid was not created"; \
	   echo "Therefore: No initrd was created and installed"; \
	fi

.PHONY: create-install-rootfs-initrd
create-install-rootfs-initrd:
ifeq ($(BUILD_IS_SORTIX),0)
	@if test -z '$(INSTALL_ROOTFS)' || test 'x$(INSTALL_ROOTFS)' = 'x/'; then \
	  echo "error: Refusing to create an initrd for the local non-Sortix operating system" >&2; \
	  exit 1; \
	fi
endif
	mkdir -p "$(INSTALL_ROOTFS)/boot"
	rm -rf "$(INSTALL_ROOTFS)/boot/sortix.initrd.d"
	mkdir -p "$(INSTALL_ROOTFS)/boot/sortix.initrd.d"
	mkdir -p "$(INSTALL_ROOTFS)/boot/sortix.initrd.d/bin"
	for PROGRAM in init mbrfs extfs; do \
	  cp "$(INSTALL_ROOTFS)/bin/$$PROGRAM" "$(INSTALL_ROOTFS)/boot/sortix.initrd.d/bin/$$PROGRAM"; \
	done
	mkdir -p "$(INSTALL_ROOTFS)/boot/sortix.initrd.d/etc"
	mkdir -p "$(INSTALL_ROOTFS)/boot/sortix.initrd.d/etc/init"
	cp "$(INSTALL_ROOTFS)/etc/rootfs.uuid" "$(INSTALL_ROOTFS)/boot/sortix.initrd.d/etc/init/rootfs.uuid"
	mkinitrd --format=sortix-initrd-2 "$(INSTALL_ROOTFS)/boot/sortix.initrd.d" -o "$(INSTALL_ROOTFS)/boot/sortix.initrd"
	rm -rf "$(INSTALL_ROOTFS)/boot/sortix.initrd.d"

.PHONY: sysmerge
sysmerge: sysroot
ifeq ($(BUILD_IS_SORTIX),0)
	if test -z '$(DESTDIR)' || test 'x$(DESTDIR)' = 'x/'; then \
	  echo "error: Refusing to corrupt the local operating system by sysmerging it with Sortix" >&2 \
	  exit 1 \
	fi
endif
	for ENTRY in $$(ls -A "$(SYSROOT)" | grep -Ev '^(src|etc|home|mnt|tmp|var)$$'); do \
		cp -RTv "$(SYSROOT)/$$ENTRY" "$(DESTDIR)/$$ENTRY" || exit $$?; \
	done

.PHONY: clean-build-tools
clean-build-tools:
	$(MAKE) -C carray clean
	$(MAKE) -C mkinitrd clean
	$(MAKE) -C tix clean

.PHONY: build-tools
build-tools:
	$(MAKE) -C carray
	$(MAKE) -C mkinitrd
	$(MAKE) -C tix

.PHONY: install-build-tools
install-build-tools:
	$(MAKE) -C carray install
	$(MAKE) -C mkinitrd install
	$(MAKE) -C tix install

.PHONY: sysroot-fsh
sysroot-fsh:
	mkdir -p "$(SYSROOT)"
	mkdir -p "$(SYSROOT)/bin"
	mkdir -p "$(SYSROOT)/boot"
	mkdir -p "$(SYSROOT)/etc"
	mkdir -p "$(SYSROOT)/etc/skel"
	mkdir -p "$(SYSROOT)/home"
	mkdir -p "$(SYSROOT)/include"
	mkdir -p "$(SYSROOT)/lib"
	mkdir -p "$(SYSROOT)/libexec"
	mkdir -p "$(SYSROOT)/mnt"
	mkdir -p "$(SYSROOT)/sbin"
	mkdir -p "$(SYSROOT)/share"
	mkdir -p "$(SYSROOT)/src"
	mkdir -p "$(SYSROOT)/tmp"
	mkdir -p "$(SYSROOT)/var"
	mkdir -p "$(SYSROOT)/var/empty"
	echo "root::0:0:root:/root:sh" > "$(SYSROOT)/etc/passwd"
	echo "root::0:root" > "$(SYSROOT)/etc/group"

.PHONY: sysroot-base-headers
sysroot-base-headers: sysroot-fsh
	export SYSROOT="$(SYSROOT)" && \
	(for D in libc libm libpthread kernel; do ($(MAKE) -C $$D install-headers DESTDIR="$(SYSROOT)") || exit $$?; done)

.PHONY: sysroot-system
sysroot-system: sysroot-fsh sysroot-base-headers
	export SYSROOT="$(SYSROOT)" && \
	(for D in $(MODULES); do ($(MAKE) -C $$D && $(MAKE) -C $$D install DESTDIR="$(SYSROOT)") || exit $$?; done)

.PHONY: sysroot-source
sysroot-source: sysroot-fsh
	cp .gitignore -t "$(SYSROOT)/src"
	cp COPYING-GPL -t "$(SYSROOT)/src"
	cp COPYING-LGPL -t "$(SYSROOT)/src"
	cp Makefile -t "$(SYSROOT)/src"
	cp README -t "$(SYSROOT)/src"
	cp -RT build-aux "$(SYSROOT)/src/build-aux"
	cp -RT isosrc "$(SYSROOT)/src/isosrc"
	(for D in $(MODULES); do (cp -R $$D -t "$(SYSROOT)/src" && $(MAKE) -C "$(SYSROOT)/src/$$D" clean) || exit $$?; done)

.PHONY: sysroot-ports
sysroot-ports: sysroot-fsh sysroot-base-headers sysroot-system sysroot-source
	@SORTIX_PORTS_DIR="$(SORTIX_PORTS_DIR)" \
	 SORTIX_REPOSITORY_DIR="$(SORTIX_REPOSITORY_DIR)" \
	 SYSROOT="$(SYSROOT)" \
	 HOST="$(HOST)" \
	 MAKE="$(MAKE)" \
	 MAKEFLAGS="$(MAKEFLAGS)" \
	 build-aux/build-ports.sh

.PHONY: sysroot-overlay
sysroot-overlay: sysroot-fsh sysroot-system sysroot-ports
	! [ -d "$(SYSROOT_OVERLAY)" ] || \
	cp -RT --preserve=mode,timestamp,links "$(SYSROOT_OVERLAY)" "$(SYSROOT)"

.PHONY: sysroot-user-skel
sysroot-user-skel: sysroot-fsh sysroot-system sysroot-ports sysroot-overlay
	cp "$(SYSROOT)/share/doc/welcome" -t "$(SYSROOT)/etc/skel"

.PHONY: sysroot-home-directory
sysroot-home-directory: sysroot-fsh sysroot-system sysroot-ports sysroot-overlay sysroot-user-skel
	mkdir -p "$(SYSROOT)/root"
	cp -RT "$(SYSROOT)/etc/skel" "$(SYSROOT)/root"

.PHONY: sysroot
sysroot: sysroot-system sysroot-source sysroot-ports sysroot-overlay sysroot-home-directory

$(SORTIX_REPOSITORY_DIR):
	mkdir -p $@

$(SORTIX_REPOSITORY_DIR)/$(HOST): $(SORTIX_REPOSITORY_DIR)
	mkdir -p $@

.PHONY: clean-core
clean-core:
	(for D in $(MODULES); do $(MAKE) clean -C $$D || exit $$?; done)

.PHONY: clean-ports
clean-ports:
	@SORTIX_PORTS_DIR="$(SORTIX_PORTS_DIR)" \
	 HOST="$(HOST)" \
	 MAKE="$(MAKE)" \
	 MAKEFLAGS="$(MAKEFLAGS)" \
	 build-aux/clean-ports.sh

.PHONY: clean-builds
clean-builds:
	rm -rf "$(SORTIX_BUILDS_DIR)"
	rm -f sortix.bin
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
most-things: sysroot initrd tar iso

.PHONY: everything
everything: most-things iso.xz

# Targets that build multiple architectures.

.PHONY: sysroot-base-headers-all-archs
sysroot-base-headers-all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) sysroot-base-headers HOST=i486-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) sysroot-base-headers HOST=x86_64-sortix

.PHONY: all-archs
all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) all HOST=i486-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) all HOST=x86_64-sortix

.PHONY: most-things-all-archs
most-things-all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) most-things HOST=i486-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) most-things HOST=x86_64-sortix

.PHONY: everything-all-archs
everything-all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) everything HOST=i486-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) everything HOST=x86_64-sortix

.PHONY: release-all-archs
release-all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) release HOST=i486-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) release HOST=x86_64-sortix

# Kernel

.PHONY: kernel
kernel: sysroot

sortix.bin: kernel
	cp "$(SYSROOT)/boot/sortix.bin" sortix.bin

# Initial ramdisk

$(INITRD): sysroot
	mkdir -p `dirname $(INITRD)`
	echo -n > $(INITRD).filter
	echo "exclude /boot" >> $(INITRD).filter
	echo "exclude /dev" >> $(INITRD).filter
	echo "exclude /next" >> $(INITRD).filter
	echo "exclude /src/sysroot" >> $(INITRD).filter
	echo "exclude /tmp" >> $(INITRD).filter
	if ! which mkinitrd; then echo You need to install mkinitrd; fi
	mkinitrd --format=sortix-initrd-2 --filter=$(INITRD).filter "$(SYSROOT)" -o $(INITRD)
	rm -f $(INITRD).filter

.PHONY: initrd
initrd: $(INITRD)

sortix.initrd: $(INITRD)
	cp $(INITRD) sortix.initrd

# Packaging

$(SORTIX_BUILDS_DIR):
	mkdir -p $(SORTIX_BUILDS_DIR)

$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).tar.xz: sysroot $(INITRD) $(SORTIX_BUILDS_DIR)
	rm -rf $(SORTIX_BUILDS_DIR)/tardir
	mkdir -p $(SORTIX_BUILDS_DIR)/tardir
	mkdir -p $(SORTIX_BUILDS_DIR)/tardir/boot
	cp "$(SYSROOT)/boot/sortix.bin" $(SORTIX_BUILDS_DIR)/tardir/boot/sortix.bin
	cp $(INITRD) $(SORTIX_BUILDS_DIR)/tardir/boot/sortix.initrd
	tar --create --xz --file $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).tar.xz -C $(SORTIX_BUILDS_DIR)/tardir `ls $(SORTIX_BUILDS_DIR)/tardir`
	rm -rf $(SORTIX_BUILDS_DIR)/tardir

.PHONY: tar
tar: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).tar.xz

# Bootable images

$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso: sysroot $(INITRD) $(SORTIX_BUILDS_DIR)
	rm -rf $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	mkdir -p $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	cp -RT isosrc $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	cp "$(SYSROOT)/boot/sortix.bin" $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/sortix.bin
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
	cp $< $@

sortix.iso.xz: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso.xz
	cp $< $@

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

$(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(BUILD_NAME).tar.xz: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).tar.xz $(SORTIX_RELEASE_DIR)/$(VERSION)/builds
	cp $< $@

.PHONY: release-tar
release-tar: $(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(BUILD_NAME).tar.xz

.PHONY: release-builds
release-builds: release-iso.xz release-tar

$(SORTIX_RELEASE_DIR)/$(VERSION)/doc: $(SORTIX_RELEASE_DIR)/$(VERSION) doc doc/*
	cp -RT doc $(SORTIX_RELEASE_DIR)/$(VERSION)/doc
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
	cp -RT $(SORTIX_REPOSITORY_DIR)/$(HOST) $@

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
