SOFTWARE_MEANT_FOR_SORTIX=1
include build-aux/platform.mak
include build-aux/compiler.mak
include build-aux/version.mak

MODULES=\
doc \
libc \
libm \
dispd \
libmount \
bench \
carray \
disked \
editor \
ext \
games \
init \
kblayout \
kblayout-compiler \
login \
mkinitrd \
regress \
sf \
sh \
sysinstall \
tix \
trianglix \
update-initrd \
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
SORTIX_ISO_COMPRESSION?=xz

SORTIX_INCLUDE_SOURCE_GIT_REPO?=$(shell test -d .git && echo "file://`pwd`")
SORTIX_INCLUDE_SOURCE_GIT_REPO:=$(SORTIX_INCLUDE_SOURCE_GIT_REPO)
SORTIX_INCLUDE_SOURCE_GIT_ORIGIN?=
SORTIX_INCLUDE_SOURCE_GIT_CLONE_OPTIONS?=--single-branch
SORTIX_INCLUDE_SOURCE_GIT_BRANCHES?=master
ifneq ($(and $(shell which git 2>/dev/null),$(SORTIX_INCLUDE_SOURCE_GIT_REPO)),)
  SORTIX_INCLUDE_SOURCE?=git
else
  SORTIX_INCLUDE_SOURCE?=yes
endif

include build-aux/dirs.mak

BUILD_NAME:=sortix-$(VERSION)-$(MACHINE)

LIVE_INITRD:=$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).live.initrd
OVERLAY_INITRD:=$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).overlay.initrd
SRC_INITRD:=$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).src.initrd
SYSTEM_INITRD:=$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).system.initrd

.PHONY: all
all: sysroot

.PHONY: sysmerge
sysmerge: sysroot
	"$(SYSROOT)/sbin/sysmerge" "$(SYSROOT)"

.PHONY: sysmerge-wait
sysmerge-wait: sysroot
	"$(SYSROOT)/sbin/sysmerge" --wait "$(SYSROOT)"

.PHONY: clean-build-tools
clean-build-tools:
	$(MAKE) -C carray clean
	$(MAKE) -C kblayout-compiler clean
	$(MAKE) -C mkinitrd clean
	$(MAKE) -C sf clean
	$(MAKE) -C tix clean

.PHONY: build-tools
build-tools:
	$(MAKE) -C carray
	$(MAKE) -C kblayout-compiler
	$(MAKE) -C mkinitrd
	$(MAKE) -C sf
	$(MAKE) -C tix

.PHONY: install-build-tools
install-build-tools:
	$(MAKE) -C carray install
	$(MAKE) -C kblayout-compiler install
	$(MAKE) -C mkinitrd install
	$(MAKE) -C sf install
	$(MAKE) -C tix install

.PHONY: sysroot-fsh
sysroot-fsh:
	mkdir -p "$(SYSROOT)"
	mkdir -p "$(SYSROOT)/bin"
	mkdir -p "$(SYSROOT)/boot"
	mkdir -p "$(SYSROOT)/dev"
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
	mkdir -p "$(SYSROOT)/tix"
	mkdir -p "$(SYSROOT)/tix/manifest"
	mkdir -p "$(SYSROOT)/tmp"
	mkdir -p "$(SYSROOT)/var"
	mkdir -p "$(SYSROOT)/var/empty"
	ln -sfT . "$(SYSROOT)/usr"

.PHONY: sysroot-base-headers
sysroot-base-headers: sysroot-fsh
	export SYSROOT="$(SYSROOT)" && \
	(for D in libc libm kernel; do ($(MAKE) -C $$D install-headers DESTDIR="$(SYSROOT)") || exit $$?; done)

.PHONY: sysroot-system
sysroot-system: sysroot-fsh sysroot-base-headers
	rm -f "$(SYSROOT)/tix/manifest/system"
	echo / >> "$(SYSROOT)/tix/manifest/system"
	echo /bin >> "$(SYSROOT)/tix/manifest/system"
	echo /boot >> "$(SYSROOT)/tix/manifest/system"
	echo /dev >> "$(SYSROOT)/tix/manifest/system"
	echo /etc >> "$(SYSROOT)/tix/manifest/system"
	echo /etc/skel >> "$(SYSROOT)/tix/manifest/system"
	echo /home >> "$(SYSROOT)/tix/manifest/system"
	echo /include >> "$(SYSROOT)/tix/manifest/system"
	echo /lib >> "$(SYSROOT)/tix/manifest/system"
	echo /libexec >> "$(SYSROOT)/tix/manifest/system"
	echo /mnt >> "$(SYSROOT)/tix/manifest/system"
	echo /sbin >> "$(SYSROOT)/tix/manifest/system"
	echo /share >> "$(SYSROOT)/tix/manifest/system"
	echo /src >> "$(SYSROOT)/tix/manifest/system"
	echo /tix >> "$(SYSROOT)/tix/manifest/system"
	echo /tix/manifest >> "$(SYSROOT)/tix/manifest/system"
	echo /tmp >> "$(SYSROOT)/tix/manifest/system"
	echo /usr >> "$(SYSROOT)/tix/manifest/system"
	echo /var >> "$(SYSROOT)/tix/manifest/system"
	echo /var/empty >> "$(SYSROOT)/tix/manifest/system"
	echo "$(HOST_MACHINE)" > "$(SYSROOT)/etc/machine"
	echo /etc/machine >> "$(SYSROOT)/tix/manifest/system"
	(echo 'NAME="Sortix"' && \
	 echo 'VERSION="$(VERSION)"' && \
	 echo 'ID=sortix' && \
	 echo 'VERSION_ID="$(VERSION)"' && \
	 echo 'PRETTY_NAME="Sortix $(VERSION)"' && \
	 echo 'SORTIX_ABI=0.0' && \
	 true) > "$(SYSROOT)/etc/sortix-release"
	echo /etc/sortix-release >> "$(SYSROOT)/tix/manifest/system"
	ln -sf sortix-release "$(SYSROOT)/etc/os-release"
	echo /etc/os-release >> "$(SYSROOT)/tix/manifest/system"
	find share | sed -e 's,^,/,' >> "$(SYSROOT)/tix/manifest/system"
	cp -RT share "$(SYSROOT)/share"
	export SYSROOT="$(SYSROOT)" && \
	(for D in $(MODULES); \
	  do ($(MAKE) -C $$D && \
	      rm -rf "$(SYSROOT).destdir" && \
	      mkdir -p "$(SYSROOT).destdir" && \
	      $(MAKE) -C $$D install DESTDIR="$(SYSROOT).destdir" && \
	      (cd "$(SYSROOT).destdir" && find .) | sed -e 's/\.//' -e 's/^$$/\//' | \
	      grep -E '^.+$$' >> "$(SYSROOT)/tix/manifest/system" && \
	      cp -RT "$(SYSROOT).destdir" "$(SYSROOT)" && \
	      rm -rf "$(SYSROOT).destdir") \
	  || exit $$?; done)
	LC_ALL=C sort -u "$(SYSROOT)/tix/manifest/system" > "$(SYSROOT)/tix/manifest/system.new"
	mv "$(SYSROOT)/tix/manifest/system.new" "$(SYSROOT)/tix/manifest/system"

.PHONY: sysroot-source
sysroot-source: sysroot-fsh
ifeq ($(SORTIX_INCLUDE_SOURCE),git)
	rm -rf "$(SYSROOT)/src"
	git clone --no-hardlinks $(SORTIX_INCLUDE_SOURCE_GIT_CLONE_OPTIONS) -- $(SORTIX_INCLUDE_SOURCE_GIT_REPO) "$(SYSROOT)/src"
	-cd "$(SYSROOT)/src" && for BRANCH in $(SORTIX_INCLUDE_SOURCE_GIT_BRANCHES); do \
	  git fetch origin $$BRANCH && \
	  (git branch -f $$BRANCH FETCH_HEAD || true) ; \
	done
ifneq ($(SORTIX_INCLUDE_SOURCE_GIT_ORIGIN),)
	cd "$(SYSROOT)/src" && git remote set-url origin $(SORTIX_INCLUDE_SOURCE_GIT_ORIGIN)
else
	-cd "$(SYSROOT)/src" && git remote rm origin
endif
else ifneq ($(SORTIX_INCLUDE_SOURCE),no)
	cp .gitignore -t "$(SYSROOT)/src"
	cp LICENSE -t "$(SYSROOT)/src"
	cp Makefile -t "$(SYSROOT)/src"
	cp README -t "$(SYSROOT)/src"
	cp -RT build-aux "$(SYSROOT)/src/build-aux"
	cp -RT share "$(SYSROOT)/src/share"
	(for D in $(MODULES); do (cp -R $$D -t "$(SYSROOT)/src" && $(MAKE) -C "$(SYSROOT)/src/$$D" clean) || exit $$?; done)
endif
	(cd "$(SYSROOT)" && find .) | sed 's/\.//' | \
	grep -E '^/src(/.*)?$$' | \
	LC_ALL=C sort > "$(SYSROOT)/tix/manifest/src"

.PHONY: sysroot-ports
sysroot-ports: sysroot-fsh sysroot-base-headers sysroot-system sysroot-source
	@SORTIX_PORTS_DIR="$(SORTIX_PORTS_DIR)" \
	 SORTIX_REPOSITORY_DIR="$(SORTIX_REPOSITORY_DIR)" \
	 SYSROOT="$(SYSROOT)" \
	 HOST="$(HOST)" \
	 MAKE="$(MAKE)" \
	 MAKEFLAGS="$(MAKEFLAGS)" \
	 build-aux/build-ports.sh

.PHONY: sysroot
sysroot: sysroot-system sysroot-source sysroot-ports

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
	rm -f sortix.iso

.PHONY: clean-release
clean-release:
	rm -rf "$(SORTIX_RELEASE_DIR)"

.PHONY: clean-repository
clean-repository:
	rm -rf "$(SORTIX_REPOSITORY_DIR)"

.PHONY: clean-sysroot
clean-sysroot:
	rm -rf "$(SYSROOT)"
	rm -rf "$(SYSROOT)".destdir

.PHONY: clean
clean: clean-core clean-ports

.PHONY: mostlyclean
mostlyclean: clean-core clean-ports clean-builds clean-release clean-sysroot

.PHONY: distclean
distclean: clean-core clean-ports clean-builds clean-release clean-repository clean-sysroot

.PHONY: most-things
most-things: sysroot iso

.PHONY: everything
everything: most-things

# Targets that build multiple architectures.

.PHONY: sysroot-base-headers-all-archs
sysroot-base-headers-all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) sysroot-base-headers HOST=i686-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) sysroot-base-headers HOST=x86_64-sortix

.PHONY: all-archs
all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) all HOST=i686-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) all HOST=x86_64-sortix

.PHONY: most-things-all-archs
most-things-all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) most-things HOST=i686-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) most-things HOST=x86_64-sortix

.PHONY: everything-all-archs
everything-all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) everything HOST=i686-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) everything HOST=x86_64-sortix

.PHONY: release-all-archs
release-all-archs:
	$(MAKE) clean clean-sysroot
	$(MAKE) release HOST=i686-sortix
	$(MAKE) clean clean-sysroot
	$(MAKE) release HOST=x86_64-sortix

# Initial ramdisk

$(LIVE_INITRD): sysroot
	mkdir -p `dirname $(LIVE_INITRD)`
	rm -rf $(LIVE_INITRD).d
	mkdir -p $(LIVE_INITRD).d
	mkdir -p $(LIVE_INITRD).d/etc
	mkdir -p $(LIVE_INITRD).d/etc/init
	echo single-user > $(LIVE_INITRD).d/etc/init/target
	echo "root::0:0:root:/root:sh" > $(LIVE_INITRD).d/etc/passwd
	echo "root::0:root" > $(LIVE_INITRD).d/etc/group
	mkdir -p $(LIVE_INITRD).d/home
	mkdir -p $(LIVE_INITRD).d/root -m 700
	cp -RT "$(SYSROOT)/etc/skel" $(LIVE_INITRD).d/root
	cp doc/welcome $(LIVE_INITRD).d/root
	tix-collection $(LIVE_INITRD).d create --platform=$(HOST) --prefix= --disable-multiarch --generation=2
	mkinitrd --format=sortix-initrd-2 $(LIVE_INITRD).d -o $(LIVE_INITRD)
	rm -rf $(LIVE_INITRD).d

.PHONY: $(OVERLAY_INITRD)
$(OVERLAY_INITRD): sysroot
	test ! -d "$(SYSROOT_OVERLAY)" || \
	mkinitrd --format=sortix-initrd-2 "$(SYSROOT_OVERLAY)" -o $(OVERLAY_INITRD)

$(SRC_INITRD): sysroot
	mkinitrd --format=sortix-initrd-2 --manifest="$(SYSROOT)/tix/manifest/src" "$(SYSROOT)" -o $(SRC_INITRD)

$(SYSTEM_INITRD): sysroot
	mkinitrd --format=sortix-initrd-2 --manifest="$(SYSROOT)/tix/manifest/system" "$(SYSROOT)" -o $(SYSTEM_INITRD)

# Packaging

$(SORTIX_BUILDS_DIR):
	mkdir -p $(SORTIX_BUILDS_DIR)

# Bootable images

$(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso: sysroot $(LIVE_INITRD) $(OVERLAY_INITRD) $(SRC_INITRD) $(SYSTEM_INITRD) $(SORTIX_BUILDS_DIR)
	rm -rf $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	mkdir -p $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	mkdir -p $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot
	mkdir -p $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/repository
	SORTIX_PORTS_DIR="$(SORTIX_PORTS_DIR)" \
	SORTIX_REPOSITORY_DIR="$(SORTIX_REPOSITORY_DIR)" \
	SYSROOT="$(SYSROOT)" \
	HOST="$(HOST)" \
	build-aux/iso-repository.sh $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/repository
ifeq ($(SORTIX_ISO_COMPRESSION),xz)
	xz -c "$(SYSROOT)/boot/sortix.bin" > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/sortix.bin.xz
	xz -c $(LIVE_INITRD) > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/live.initrd.xz
	test ! -e "$(OVERLAY_INITRD)" || \
	xz -c $(OVERLAY_INITRD) > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/overlay.initrd.xz
	xz -c $(SRC_INITRD) > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/src.initrd.xz
	xz -c $(SYSTEM_INITRD) > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/system.initrd.xz
	build-aux/iso-grub-cfg.sh --platform $(HOST) --version $(VERSION) $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	grub-mkrescue --compress=xz -o $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
else ifeq ($(SORTIX_ISO_COMPRESSION),gzip)
	gzip -c "$(SYSROOT)/boot/sortix.bin" > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/sortix.bin.gz
	gzip -c $(LIVE_INITRD) > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/live.initrd.gz
	test ! -e "$(OVERLAY_INITRD)" || \
	gzip -c $(OVERLAY_INITRD) > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/overlay.initrd.gz
	gzip -c $(SRC_INITRD) > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/src.initrd.gz
	gzip -c $(SYSTEM_INITRD) > $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/system.initrd.gz
	build-aux/iso-grub-cfg.sh --platform $(HOST) --version $(VERSION) $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	grub-mkrescue --compress=gz -o $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
else # none
	cp "$(SYSROOT)/boot/sortix.bin" $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/sortix.bin
	cp $(LIVE_INITRD) $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/live.initrd
	test ! -e "$(OVERLAY_INITRD)" || \
	cp $(OVERLAY_INITRD) $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/overlay.initrd
	cp $(SRC_INITRD) $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/src.initrd
	cp $(SYSTEM_INITRD) $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso/boot/system.initrd
	build-aux/iso-grub-cfg.sh --platform $(HOST) --version $(VERSION) $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
	grub-mkrescue -o $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso
endif
	rm -rf $(SORTIX_BUILDS_DIR)/$(BUILD_NAME)-iso

.PHONY: iso
iso: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso

sortix.iso: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso
	cp $< $@

# Release

$(SORTIX_RELEASE_DIR)/$(VERSION):
	mkdir -p $@

$(SORTIX_RELEASE_DIR)/$(VERSION)/builds: $(SORTIX_RELEASE_DIR)/$(VERSION)
	mkdir -p $@

$(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(BUILD_NAME).iso: $(SORTIX_BUILDS_DIR)/$(BUILD_NAME).iso $(SORTIX_RELEASE_DIR)/$(VERSION)/builds
	cp $< $@

.PHONY: release-iso
release-iso: $(SORTIX_RELEASE_DIR)/$(VERSION)/builds/$(BUILD_NAME).iso

.PHONY: release-builds
release-builds: release-iso

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
release-arch: release-builds release-readme release-repository

.PHONY: release-shared
release-shared: release-readme

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
	wc -l `find . -type f | grep -Ev '\.(h|h\+\+|c$$|cpp|c\+\+|s|S|asm|kblayout|mak|sh)$$|(^|/)Makefile$$' | grep -Ev '^\./(\.git|sysroot|sysroot-overlay|ports|repository)(/|$$)'` | sort -n
