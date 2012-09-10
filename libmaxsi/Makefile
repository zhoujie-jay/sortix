include ../compiler.mak
include ../version.mak
include ../dirs.mak

ifndef OPTLEVEL
    OPTLEVEL:=-O2
endif

ifeq ($(DEBUG_LIBC),1)
    FLAGSDEBUG=-g3
else
    FLAGSDEBUG=$(OPTLEVEL)
endif

CPUDIR:=$(CPU)

ifeq ($(HOST),i486-sortix)
    CPUFLAGS=
endif

ifeq ($(HOST),x86_64-sortix)
    CPUFLAGS=-fPIC -mno-red-zone
endif

CPPINCLUDES=-I preproc
CPPFLAGS=-DLIBMAXSI_LIBRARY -U_GNU_SOURCE $(CPUDEFINES) $(CPPINCLUDES)
FLAGS=$(CPUFLAGS) -Wall -Wextra -fno-stack-protector $(FLAGSDEBUG)
CFLAGS=$(FLAGS) -std=c99
CXXFLAGS=$(FLAGS) -std=gnu++0x -fno-exceptions -fno-rtti
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
$(CPUDIR)/signal.o \
$(CPUDIR)/fork.o \
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

BINS=libc.a libg.a libm.a libstdc++.a libmaxsi.a libmaxsi-sortix.a $(CRTOBJ)

all: $(BINS)

.PHONY: all headers clean install install-include-dirs install-headers \
        install-lib-dirs install-libs

libmaxsi.a: $(OBJS)
	$(HOSTAR) rcs libmaxsi.a $(OBJS)

libmaxsi-sortix.a: $(SORTIXOBJS)
	$(HOSTAR) rcs libmaxsi-sortix.a $(SORTIXOBJS)

libc.a: libmaxsi.a
	ln -sf $< $@

libg.a: libc.a
	ln -sf $< $@

libm.a:
	$(HOSTAR) rcs $@

libstdc++.a:
	$(HOSTAR) rcs $@

start.o: $(CPUDIR)/start.o
	ln -f $< $@

crt1.o: $(CPUDIR)/crt1.o
	ln -f $< $@

crti.o: $(CPUDIR)/crti.o
	ln -f $< $@

crtn.o: $(CPUDIR)/crtn.o
	ln -f $< $@

# header preprocessing
$(HEADERDIRS):
	mkdir -p $@

preproc/%: include/% $(HEADERDIRS)
	mxmpp -I decl $< -o $@

headers: $(HEADERDIRS) $(HEADERS)

# standard library
%.o: %.c headers
	$(HOSTCC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS)

%.o: %.cpp headers
	$(HOSTCXX) -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

%.o: %.s
	$(HOSTAS) $(ASFLAGS) $< -o $@

# libmaxsi-sortix
sortix:
	mkdir -p sortix

sortix/%.o: %.cpp $(HEADERS) sortix
	$(HOSTCXX) -c $< -o $@ $(CPPFLAGS) $(SORTIXCPPFLAGS) $(CXXFLAGS)

clean:
	rm -f *.o sortix/*.o c/*.o x86/*.o x64/*.o *.a *.so
	rm -f $(OBJS)
	rm -rf sortix $(HEADERDIRS)

# Installation into sysroot
install: install-headers install-libs

install-include-dirs: headers
	mkdir -p $(INSTALLINCLUDEDIR)
	DIRS=$$(echo $(UNPROCHEADERDIRS) | \
	        tr ' ' '\n' | \
	        sed 's/include\/*//'); \
	for D in $$DIRS; do \
	  mkdir -p $(INSTALLINCLUDEDIR)/$$D || exit $?; \
	done

install-headers: install-include-dirs headers
	for SRC in $(HEADERS); do \
	  DEST=`echo $$SRC | sed 's/preproc\///'`; \
	  cp -P $$SRC $(INSTALLINCLUDEDIR)/$$DEST || exit $?; \
	done

install-lib-dirs:
	mkdir -p $(INSTALLLIBDIR)
	mkdir -p $(INSTALLLIBDIR)/$(HOST)

install-libs: install-lib-dirs
	for F in $(BINS); do \
	  cp -P $$F $(INSTALLLIBDIR)/$(HOST) || exit $?; \
	done

