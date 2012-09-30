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
CPPFLAGS=-DLIBC_LIBRARY -U_GNU_SOURCE $(CPUDEFINES) $(CPPINCLUDES)
FLAGS=$(CPUFLAGS) -Wall -Wextra -fno-stack-protector $(FLAGSDEBUG)
CFLAGS=$(FLAGS) -std=c99
CXXFLAGS=$(FLAGS) -std=gnu++0x -fno-exceptions -fno-rtti
ASFLAGS=$(CPUASFLAGS)

FREEOBJS=\
abort.o \
abs.o \
_assert.o \
atoi.o \
atoll.o \
atol.o \
bsearch.o \
calloc.o \
clearerr.o \
c++.o \
ctime.o \
ctype.o \
dir.o \
errno.o \
fabs.o \
fbufsize.o \
fclose.o \
feof.o \
ferror.o \
fflush.o \
fgetc.o \
fgets.o \
flbf.o \
flushlfb.o \
fnewline.o \
format.o \
fpending.o \
fpurge.o \
fputc.o \
fputs.o \
freadable.o \
freading.o \
fread.o \
fregister.o \
fscanf.o \
fseek.o \
fseeko.o \
fseterr.o \
fsetlocking.o \
ftell.o \
ftello.o \
fwritable.o \
fwrite.o \
fwriting.o \
getdelim.o \
getline.o \
gmtime.o \
gmtime_r.o \
heap.o \
integer.o \
localtime.o \
localtime_r.o \
mbtowc.o \
memchr.o \
memcmp.o \
memcpy.o \
memmove.o \
memset.o \
op-new.o \
readparamstring.o \
rewind.o \
sort.o \
sprint.o \
sscanf.o \
stpcpy.o \
strcasecmp.o \
strcat.o \
strchrnul.o \
strchr.o \
strcmp.o \
strcoll.o \
strcpy.o \
strcspn.o \
strdup.o \
strerror.o \
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
ungetc.o \
vfscanf.o \
vsscanf.o \

HOSTEDOBJS=\
access.o \
chdir.o \
chmod.o \
clock.o \
close.o \
$(CPUDIR)/fork.o \
$(CPUDIR)/signal.o \
dlfcn.o \
dup.o \
env.o \
errorprint.o \
execle.o \
execl.o \
execlp.o \
execve.o \
execv.o \
execvpe.o \
execvp.o \
_exit.o \
_Exit.o \
exit.o \
fchmod.o \
fcloseall.o \
fcntl.o \
fddir-sortix.o \
fdio.o \
fileno.o \
fork.o \
fpipe.o \
fstat.o \
ftruncate.o \
getc.o \
getcwd.o \
getdtablesize.o \
getpagesize.o \
getpid.o \
getppid.o \
gettermmode.o \
gettimeofday.o \
init.o \
ioleast.o \
isatty.o \
kernelinfo.o \
kill.o \
localeconv.o \
lseek.o \
memstat.o \
mkdir.o \
mktemp.o \
on_exit.o \
open.o \
openat.o \
pipe.o \
print.o \
putc.o \
raise.o \
rand.o \
readdirents.o \
read.o \
rmdir.o \
sbrk.o \
scanf.o \
setjmp.o \
setlocale.o \
settermmode.o \
sfork.o \
SIG_DFL.o \
SIG_ERR.o \
SIG_IGN.o \
signal.o \
sleep.o \
stat.o \
stdio.o \
tfork.o \
time.o \
truncate.o \
umask.o \
unlink.o \
uptime.o \
usleep.o \
utime.o \
vexecle.o \
vexecl.o \
vexeclp.o \
vscanf.o \
wait.o \
waitpid.o \
winsize.o \
write.o \

OBJS=\
$(FREEOBJS) \
$(HOSTEDOBJS) \

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

SORTIXOBJS:=$(addprefix sortix/,$(FREEOBJS))
SORTIXCPPFLAGS:=-DSORTIX_KERNEL

BINS=libc.a libg.a libm.a libstdc++.a libc-sortix.a $(CRTOBJ)

all: $(BINS)

.PHONY: all headers clean install install-include-dirs install-headers \
        install-lib-dirs install-libs

libc.a: $(OBJS)
	$(HOSTAR) rcs $@ $(OBJS)

libc-sortix.a: $(SORTIXOBJS)
	$(HOSTAR) rcs $@ $(SORTIXOBJS)

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

# libc-sortix
sortix:
	mkdir -p sortix

sortix/%.o: %.c $(HEADERS) sortix
	$(HOSTCC) -c $< -o $@ $(CPPFLAGS) $(SORTIXCPPFLAGS) $(CFLAGS)

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
