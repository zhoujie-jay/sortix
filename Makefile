ifndef CPU
    CPU=x86
endif

REMOTE=192.168.2.6
REMOTEUSER=sortie
REMOTECOPYDIR:=/home/$(REMOTEUSER)/Desktop/MaxsiOS
MODULES=libmaxsi hello pong mkinitrd sortix

VERSION=0.4
DEBNAME:=sortix_$(VERSION)_$(CPU)
DEBSRCNAME:=sortix_$(VERSION)
DEBDIR:=builds/$(DEBNAME)
DEBSRCDIR:=builds/$(DEBSRCNAME)-src
DEBFILE:=builds/$(DEBNAME).deb
PACKAGENAME:=sortix
ISODIR:=builds/$(DEBNAME)-iso
ISOFILE:=builds/$(DEBNAME).iso
JSNAME:=jssortix_$(VERSION)_$(CPU).bin
INITRDDIR:=initrd

all:
	(for D in $(MODULES); do $(MAKE) all $(MFLAGS) --directory $$D; done)
clean: 
	(for D in $(MODULES); do $(MAKE) clean $(MFLAGS) --directory $$D; done)

distclean: clean cleanbuilds

cleanbuilds:
	rm -rf builds/
	rm -f sortix.iso

everything: all deb iso jssortix

# Statistics
linecount:
	wc -l `find | grep -E '\.h$$|\.c$$|\.cpp$$|\.s$$|\.asm$$|Makefile$$'` | sort -n

# Local machine

install: all
	cp sortix/sortix.bin /boot
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

jssortix: all
	mkdir -p builds
	$(MAKE) jssortix $(MFLAGS) --directory sortix
	cp sortix/jssortix.bin builds/$(JSNAME)

# Bootable images

iso: all debsource
	rm -rf $(ISODIR)
	mkdir -p builds
	mkdir -p $(ISODIR)
	cp -r isosrc/. $(ISODIR)
	cp sortix/sortix.bin $(ISODIR)/boot
	mkdir -p $(INITRDDIR)
	cp hello/hello $(INITRDDIR)
	cp pong/pong $(INITRDDIR)
	cp $(INITRDDIR)/hello $(INITRDDIR)/init
	(cd $(INITRDDIR) && ../mkinitrd/mkinitrd * -o ../$(ISODIR)/boot/sortix.initrd)
	rm -rf $(INITRDDIR)
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


