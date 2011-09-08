ifndef CPU
    CPU=x86
endif

REMOTE=192.168.2.6
REMOTEUSER=sortie
REMOTECOPYDIR:=/home/$(REMOTEUSER)/Desktop/MaxsiOS
MODULES=libmaxsi hello games mkinitrd utils sortix

VERSION=0.4
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
	(for D in $(MODULES); do $(MAKE) all $(MFLAGS) --directory $$D || exit $?; done)

clean:
	rm -f $(INITRD)
	(for D in $(MODULES); do $(MAKE) clean $(MFLAGS) --directory $$D || exit $?; done)

distclean: clean cleanbuilds

cleanbuilds:
	rm -rf builds/
	rm -f sortix.iso

everything: all deb iso

# Initializing RamDisk
$(INITRD): suball
	(cd $(INITRDDIR) && ../mkinitrd/mkinitrd * -o ../$(INITRD))

# Statistics
linecount:
	wc -l `find | grep -E '\.h$$|\.c$$|\.cpp$$|\.s$$|\.asm$$|Makefile$$'` | sort -n

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
	cp -r debsrc/. $(DEBDIR)
	mkdir -p  $(DEBDIR)/boot
	cp sortix/sortix.bin $(DEBDIR)/boot
	cp sortix/sortix.initrd $(DEBDIR)/boot
	cat debsrc/DEBIAN/control | \
	sed "s/SORTIX_PACKAGE_NAME/$(PACKAGENAME)/g" | \
	sed "s/SORTIX_VERSION/$(VERSION)/g" | \
	sed "s/SORTIX_ARCH/all/g" | \
	cat > $(DEBDIR)/DEBIAN/control
	dpkg --build $(DEBDIR) $(DEBFILE)
	rm -rf $(DEBDIR)/DEBIAN
	(cd builds/$(DEBNAME) && tar cfzv ../$(DEBNAME).tar.gz `ls`)
	rm -rf $(DEBDIR)

debsource: all
	rm -rf $(DEBSRCDIR)
	mkdir -p $(DEBSRCDIR)
	for D in `ls | grep -v builds`; do cp -r $$D $(DEBSRCDIR); done
	(cd $(DEBSRCDIR) && make distclean)
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


