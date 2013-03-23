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
atof.o \
atoi.o \
atoll.o \
atol.o \
bsearch.o \
calloc.o \
clearerr.o \
c++.o \
ctype.o \
dir.o \
div.o \
errno.o \
fabs.o \
fbufsize.o \
fclose.o \
fdeletefile.o \
feof.o \
ferror.o \
fflush.o \
fflush_stop_reading.o \
fflush_stop_writing.o \
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
fresetfile.o \
fscanf.o \
fseek.o \
fseeko.o \
fsetdefaultbuf.o \
fseterr.o \
fsetlocking.o \
fshutdown.o \
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
ldiv.o \
lldiv.o \
localtime.o \
localtime_r.o \
mbrtowc.o \
mbtowc.o \
memccpy.o \
memchr.o \
memcmp.o \
memcpy.o \
memmove.o \
memset.o \
op-new.o \
rewind.o \
setbuf.o \
setvbuf.o \
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
time/asctime.o \
time/asctime_r.o \
time/ctime.o \
time/ctime_r.o \
timespec.o \
time/strftime.o \
ungetc.o \
vfscanf.o \
vsscanf.o \
wcrtomb.o \
wcscat.o \
wcschrnul.o \
wcschr.o \
wcscmp.o \
wcscpy.o \
wcslen.o \
wcsncat.o \
wctomb.o \
wctype.o \

HOSTEDOBJS=\
access.o \
arpa/inet/inet_addr.o \
arpa/inet/inet_ntoa.o \
arpa/inet/inet_ntop.o \
arpa/inet/inet_pton.o \
calltrace.o \
canonicalize_file_name_at.o \
canonicalize_file_name.o \
chdir.o \
chmod.o \
chown.o \
clock.o \
close.o \
$(CPUDIR)/calltrace.o \
$(CPUDIR)/fork.o \
$(CPUDIR)/signal.o \
$(CPUDIR)/syscall.o \
creat.o \
dispmsg_issue.o \
dlfcn.o \
dup2.o \
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
faccessat.o \
fchdir.o \
fchmodat.o \
fchmod.o \
fchownat.o \
fchown.o \
fcloseall.o \
fcntl.o \
fddir-sortix.o \
fdio.o \
fileno.o \
fork.o \
fpipe.o \
freopen.o \
fsm_bootstraprootfd.o \
fsm_closechannel.o \
fsm_closeserver.o \
fsm_fsbind.o \
fsm_listen.o \
fsm_mkserver.o \
fsm_recv.o \
fsm_send.o \
fstatat.o \
fstat.o \
fsync.o \
ftruncate.o \
futimens.o \
getc.o \
getcwd.o \
getdtablesize.o \
getegid.o \
geteuid.o \
getgid.o \
getpagesize.o \
getpid.o \
getppid.o \
gettermmode.o \
gettimeofday.o \
getuid.o \
grent.o \
init.o \
ioctl.o \
ioleast.o \
isatty.o \
kernelinfo.o \
kill.o \
lchown.o \
linkat.o \
link.o \
localeconv.o \
lseek.o \
lstat.o \
memstat.o \
mkdirat.o \
mkdir.o \
mktemp.o \
netdb/endhostent.o \
netdb/endnetent.o \
netdb/endprotoent.o \
netdb/endservent.o \
netdb/freeaddrinfo.o \
netdb/gai_strerror.o \
netdb/getaddrinfo.o \
netdb/gethostent.o \
netdb/getnameinfo.o \
netdb/getnetbyaddr.o \
netdb/getnetbyname.o \
netdb/getnetent.o \
netdb/getprotobyname.o \
netdb/getprotobynumber.o \
netdb/getprotoent.o \
netdb/getservbyname.o \
netdb/getservbyport.o \
netdb/getservent.o \
netdb/sethostent.o \
netdb/setnetent.o \
netdb/setprotoent.o \
netdb/setservent.o \
on_exit.o \
openat.o \
open.o \
pathconf.o \
pipe.o \
poll.o \
popen.o \
ppoll.o \
print.o \
putc.o \
pwent.o \
raise.o \
rand.o \
readdirents.o \
readlinkat.o \
readlink.o \
read.o \
realpath.o \
removeat.o \
remove.o \
renameat.o \
rename.o \
rmdir.o \
sbrk.o \
scanf.o \
select.o \
setegid.o \
seteuid.o \
setgid.o \
setjmp.o \
setlocale.o \
settermmode.o \
setuid.o \
sfork.o \
SIG_DFL.o \
SIG_ERR.o \
SIG_IGN.o \
signal.o \
sleep.o \
stat.o \
stdio.o \
sys/socket/accept4.o \
sys/socket/accept.o \
sys/socket/bind.o \
sys/socket/connect.o \
sys/socket/getpeername.o \
sys/socket/getsockname.o \
sys/socket/getsockopt.o \
sys/socket/listen.o \
sys/socket/recvfrom.o \
sys/socket/recvmsg.o \
sys/socket/recv.o \
sys/socket/sendmsg.o \
sys/socket/send.o \
sys/socket/sendto.o \
sys/socket/setsockopt.o \
sys/socket/shutdown.o \
sys/socket/sockatmark.o \
sys/socket/socket.o \
sys/socket/socketpair.o \
system.o \
tfork.o \
time.o \
truncateat.o \
truncate.o \
ttyname.o \
umask.o \
unlinkat.o \
unlink.o \
uptime.o \
usleep.o \
utimensat.o \
utimens.o \
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

