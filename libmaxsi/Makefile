ifndef CPU
    CPU=x86
endif

ifeq ($(CPU),x86)
    CPUDEFINES=-DPLATFORM_X86
    CPUFLAGS=-m32
    CPULDFLAGS=-melf_i386
    CPUASFLAGS=-32
endif

ifeq ($(CPU),x64)
    CPU=x64
    CPUDEFINES=-DPLATFORM_X64
    CPUFLAGS=-fPIC -m64 -mno-red-zone
    CPULDFLAGS=-melf_x86_64
    CPUASFLAGS=-64
endif

CPPINCLUDES=-I preproc -I ../sortix/include
CPPFLAGS=-DLIBMAXSI_LIBRARY -DSORTIX -U_GNU_SOURCE $(CPUDEFINES) $(CPPINCLUDES)
FLAGS=$(CPUFLAGS) -Wall -Wextra -nostdlib -fno-builtin -nostartfiles \
                  -nodefaultlibs -fno-stack-protector -nostdinc
CFLAGS=$(FLAGS) -std=c99
CXXFLAGS=$(FLAGS) -std=gnu++0x -fno-exceptions -nostdinc++ -fno-rtti
LDFLAGS=$(CPULDFLAGS)
ASFLAGS=$(CPUASFLAGS)

OBJS=\
ctype.o \
crc32.o \
fdio.o \
fpipe.o \
stdio.o \
dir.o \
fddir-sortix.o \
setjmp.o \
setlocale.o \
sortix-sound.o \
readparamstring.o \
process.o \
thread.o \
ioleast.o \
winsize.o \
gettermmode.o \
settermmode.o \
isatty.o \
kernelinfo.o \
init.o \
signal.o \
$(CPU)/signal.o \
$(CPU)/fork.o \
time.o \
random.o \
abs.o \
env.o \
dlfcn.o \
integer.o \
c++.o \
memory.o \
heap.o \
sort.o \
string.o \
error.o \
format.o \
abort.o \
access.o \
_assert.o \
bsearch.o \
chdir.o \
chmod.o \
close.o \
dup.o \
errorprint.o \
exit.o \
_exit.o \
_Exit.o \
fabs.o \
fchmod.o \
fcntl.o \
fstat.o \
ftruncate.o \
getcwd.o \
getdtablesize.o \
localeconv.o \
lseek.o \
mbtowc.o \
mkdir.o \
mktemp.o \
on_exit.o \
open.o \
pipe.o \
print.o \
read.o \
readdirents.o \
rmdir.o \
scanf.o \
fscanf.o \
sscanf.o \
vscanf.o \
vfscanf.o \
vsscanf.o \
stat.o \
truncate.o \
umask.o \
unlink.o \
write.o \
stpcpy.o \
strcasecmp.o \
strcat.o \
strchr.o \
strchrnul.o \
strcmp.o \
strcoll.o \
strcpy.o \
strcspn.o \
strdup.o \
strlen.o \
strncasecmp.o \
strncat.o \
strncmp.o \
strncpy.o \
strnlen.o \
strpbrk.o \
strrchr.o \
strspn.o \
strstr.o \
strtok.o \
strtok_r.o \
clearerr.o \
fbufsize.o \
fclose.o \
fcloseall.o \
feof.o \
ferror.o \
fflush.o \
fgetc.o \
fgets.o \
fileno.o \
flbf.o \
flushlfb.o \
fnewline.o \
fpending.o \
fpurge.o \
fputc.o \
fputs.o \
fread.o \
freadable.o \
freading.o \
fregister.o \
fseek.o \
fseeko.o \
fseterr.o \
fsetlocking.o \
ftell.o \
ftello.o \
fwritable.o \
fwrite.o \
fwriting.o \
getc.o \
putc.o \
rewind.o \
ungetc.o \

CRTOBJ=\
start.o \
crt1.o \
crti.o \
crtn.o \

MISCOBJ=\
$(CRTOBJ) \

UNPROCHEADERDIRS:=$(shell find include -type d)
UNPROCHEADERS:=$(shell find include -type f)
HEADERDIRS:=$(patsubst include%,preproc%,$(UNPROCHEADERDIRS))
HEADERS:=$(patsubst include%,preproc%,$(UNPROCHEADERS))

SORTIXOBJS=\
c++.o \
memory.o \
heap.o \
string.o \
error.o \
format.o \
crc32.o \

SORTIXOBJS:=$(addprefix sortix/,$(SORTIXOBJS))
SORTIXCPPFLAGS:=-DSORTIX_KERNEL

BINS=libc.a libg.a libmaxsi.a libmaxsi-sortix.a $(CRTOBJ)

all: $(BINS)

.PHONY: all headers clean install install-include-dirs install-headers \
        install-lib-dirs install-libs

libmaxsi.a: $(OBJS)
	ar rcs libmaxsi.a $(OBJS)

libmaxsi.so: $(OBJS)
	ld $(LDFLAGS) -shared -o $@ $(OBJS)

libmaxsi-sortix.a: $(SORTIXOBJS)
	ar rcs libmaxsi-sortix.a $(SORTIXOBJS)

libc.a: libmaxsi.a
	ln -sf $< $@

libc.so: libmaxsi.so
	ln -sf $< $@

libg.a: libc.a
	ln -sf $< $@

libg.so: libc.so
	ln -sf $< $@

start.o: $(CPU)/start.o
	ln -f $< $@

crt1.o: $(CPU)/crt1.o
	ln -f $< $@

crti.o: $(CPU)/crti.o
	ln -f $< $@

crtn.o: $(CPU)/crtn.o
	ln -f $< $@

# header preprocessing
$(HEADERDIRS):
	mkdir -p $@

preproc/%: include/% $(HEADERDIRS)
	mxmpp -I decl $< -o $@

headers: $(HEADERDIRS) $(HEADERS)

# standard library
%.o: %.c headers
	gcc -c $< -o $@ $(CPPFLAGS) $(CFLAGS)

%.o: %.cpp headers
	g++ -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

%.o: %.s
	as $(ASFLAGS) $< -o $@

# libmaxsi-sortix
sortix:
	mkdir -p sortix

sortix/%.o: %.cpp $(HEADERS) sortix
	g++ -c $< -o $@ $(CPPFLAGS) $(SORTIXCPPFLAGS) $(CXXFLAGS)

clean:
	rm -f *.o sortix/*.o c/*.o x86/*.o x64/*.o *.a *.so
	rm -f $(OBJS)
	rm -rf sortix $(HEADERDIRS)

# Installation into sysroot
install: install-headers install-libs

install-include-dirs: headers
	mkdir -p $(SYSROOT)/usr/include
	for D in $(UNPROCHEADERDIRS); do mkdir -p $(SYSROOT)/usr/$$D || exit $?; done

install-headers: install-include-dirs headers
	for SRC in $(HEADERS); do DEST=`echo $$SRC | sed 's/preproc/include/'`; cp $$SRC $(SYSROOT)/usr/$$DEST || exit $?; done

install-lib-dirs: all
	mkdir -p $(SYSROOT)/usr/lib

install-libs: install-lib-dirs all
	for F in $(BINS); do cp -P $$F $(SYSROOT)/usr/lib || exit $?; done
	touch deleteme.cpp
	g++ $(CPUFLAGS) -c deleteme.cpp -o deleteme.o
	for F in libgcc.so libm.so libstdc++.so; do ld $(CPULDFLAGS) -shared deleteme.o -o $(SYSROOT)/usr/lib/$$F; done
	rm -f deleteme.o deleteme.cpp

