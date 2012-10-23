SOFTWARE_MEANT_FOR_SORTIX=1
include ../compiler.mak
include ../version.mak
include ../dirs.mak

ifndef OPTLEVEL
    OPTLEVEL:=-g -O2 -fno-omit-frame-pointer
endif

CPUDIR:=$(CPU)

CPPINCLUDES=-I preproc
CPPFLAGS=-DLIBC_LIBRARY $(CPPINCLUDES)
FLAGS=-Wall -Wextra $(OPTLEVEL)
CFLAGS=-std=gnu99
CXXFLAGS=-std=gnu++11 -fno-exceptions -fno-rtti
ASFLAGS=

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
flushlbf.o \
fnewfile.o \
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
memccpy.o \
memchr.o \
memcmp.o \
memcpy.o \
memmove.o \
memset.o \
op-new.o \
rewind.o \
sort.o \
sprint.o \
sscanf.o \
stpcpy.o \
stpncpy.o \
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
strndup.o \
strnlen.o \
strpbrk.o \
strrchr.o \
strsignal.o \
strspn.o \
strstr.o \
strtok.o \
strtok_r.o \
strxfrm.o \
ungetc.o \
vfscanf.o \
vsscanf.o \

HOSTEDOBJS=\
access.o \
chdir.o \
chmod.o \
chown.o \
clock.o \
close.o \
$(CPUDIR)/fork.o \
$(CPUDIR)/signal.o \
$(CPUDIR)/syscall.o \
dispmsg_issue.o \
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
fstatat.o \
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
link.o \
localeconv.o \
lseek.o \
memstat.o \
mkdir.o \
mktemp.o \
on_exit.o \
openat.o \
open.o \
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
vscanf.o \
wait.o \
waitpid.o \
winsize.o \
write.o \

OBJS=\
$(FREEOBJS) \
$(HOSTEDOBJS) \

CRTOBJ=\
crt1.o \
crti.o \
crtn.o \

MISCOBJ=\
$(CRTOBJ) \

UNPROCHEADERDIRS:=$(shell find include -type d)
UNPROCHEADERS:=$(shell find include -type f)
HEADERDIRS:=$(patsubst include%,preproc%,$(UNPROCHEADERDIRS))
HEADERS:=$(patsubst include%,preproc%,$(UNPROCHEADERS))
INSTALLHEADERDIRS:=$(addprefix $(DESTDIR)$(INCLUDEDIR),$(patsubst preproc%,%,$(HEADERDIRS)))
INSTALLHEADERS:=$(addprefix $(DESTDIR)$(INCLUDEDIR),$(patsubst preproc%,%,$(HEADERS)))

SORTIXOBJS:=$(addprefix sortix/,$(FREEOBJS))
SORTIXCPPFLAGS:=$(CPPFLAGS) -DSORTIX_KERNEL
SORTIXFLAGS:=$(FLAGS) -ffreestanding
SORTIXCFLAGS:=$(CFLAGS)
SORTIXCXXFLAGS:=$(CXXFLAGS)
ifeq ($(HOST),x86_64-sortix)
    SORTIXFLAGS:=$(SORTIXFLAGS) -mno-red-zone
endif

BINS=libc.a libg.a libm.a libpthread.a libstdc++.a $(CRTOBJ)
BINSKERNEL=libc-sortix.a
INSTALLLIBS:=$(addprefix $(DESTDIR)$(LIBDIR)/,$(BINS))
INSTALLLIBSKERNEL:=$(addprefix $(DESTDIR)$(LIBDIR)/,$(BINSKERNEL))

all: libs libs-kernel

libs: $(BINS)

libs-kernel: $(BINSKERNEL)

.PHONY: all libs headers clean install install-include-dirs install-headers \
        install-lib-dirs install-libs libs-kernel

FORCE:

libc.a: $(OBJS)
	$(HOSTAR) rcs $@ $(OBJS)

libc-sortix.a: $(SORTIXOBJS)
	$(HOSTAR) rcs $@ $(SORTIXOBJS)

libg.a:
	$(HOSTAR) rcs $@

libm.a:
	$(HOSTAR) rcs $@

libpthread.a:
	$(HOSTAR) rcs $@

libstdc++.a:
	$(HOSTAR) rcs $@

crt1.o: $(CPUDIR)/crt1.o
	ln -f $< $@

crti.o: $(CPUDIR)/crti.o
	ln -f $< $@

crtn.o: $(CPUDIR)/crtn.o
	ln -f $< $@

# header preprocessing
$(OBJS) $(SORTIXOBJS): $(HEADERS)

$(HEADERDIRS):
	mkdir -p $@

preproc/%: include/%
	mxmpp -I decl $< -o $@

$(HEADERS): | $(HEADERDIRS)

headers: $(HEADERDIRS) $(HEADERS)

# standard library
%.o: %.c
	$(HOSTCC) -c $< -o $@ $(CPPFLAGS) $(FLAGS) $(CFLAGS)

%.o: %.cpp
	$(HOSTCXX) -c $< -o $@ $(CPPFLAGS) $(FLAGS) $(CXXFLAGS)

%.o: %.s
	$(HOSTAS) $(ASFLAGS) $< -o $@

# libc-sortix
sortix/%.o: %.c
	$(HOSTCC) -c $< -o $@ $(SORTIXCPPFLAGS) $(SORTIXFLAGS) $(SORTIXCFLAGS)

sortix/%.o: %.cpp
	$(HOSTCXX) -c $< -o $@ $(SORTIXCPPFLAGS) $(SORTIXFLAGS) $(SORTIXCXXFLAGS)

clean:
	rm -f $(wildcard *.o) $(wildcard */*.o) $(wildcard */*/*.o) $(wildcard *.a)
	rm -rf $(HEADERDIRS)

# Installation into sysroot
install: install-headers install-libs install-libs-kernel

$(INSTALLHEADERDIRS) $(DESTDIR)$(LIBDIR):
	mkdir -p $@

$(DESTDIR)$(INCLUDEDIR)/%: preproc/%
	cp $< $@

$(INSTALLHEADERS): | $(INSTALLHEADERDIRS)

$(DESTDIR)$(LIBDIR)/%: %
	cp -P $< $@

$(INSTALLLIBS): | $(DESTDIR)$(LIBDIR)

$(INSTALLLIBSKERNEL): | $(DESTDIR)$(LIBDIR)

$(DESTDIR)$(INCLUDEDIR) $(DESTDIR)$(LIBDIR) $(INSTALLHEADERS): FORCE

install-include-dirs: $(INSTALLHEADERDIRS)

install-headers: $(INSTALLHEADERDIRS) $(INSTALLHEADERS)

install-lib-dirs: $(DESTDIR)$(LIBDIR)

install-libs: $(INSTALLLIBS)

install-libs-kernel: $(INSTALLLIBSKERNEL)

