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
dirent/alphasort.o \
dirent/dir.o \
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
heap.o \
integer.o \
ldiv.o \
lldiv.o \
mblen.o \
mbstowcs.o \
mbtowc.o \
op-new.o \
rewind.o \
setbuf.o \
setvbuf.o \
sigaddset.o \
sigdelset.o \
sigemptyset.o \
sigfillset.o \
sigismember.o \
sort.o \
sprint.o \
sscanf.o \
string/memccpy.o \
string/memchr.o \
string/memcmp.o \
string/memcpy.o \
string/memmove.o \
string/memset.o \
string/stpcpy.o \
string/stpncpy.o \
string/strcasecmp.o \
string/strcat.o \
string/strchrnul.o \
string/strchr.o \
string/strcmp.o \
string/strcoll.o \
string/strcpy.o \
string/strcspn.o \
string/strdup.o \
string/strerror.o \
string/strlen.o \
string/strncasecmp.o \
string/strncat.o \
string/strncmp.o \
string/strncpy.o \
string/strndup.o \
string/strnlen.o \
string/strpbrk.o \
string/strrchr.o \
string/strsignal.o \
string/strspn.o \
string/strstr.o \
string/strtok.o \
string/strtok_r.o \
string/strxfrm.o \
strtod.o \
strtof.o \
strtold.o \
time/asctime.o \
time/asctime_r.o \
time/gmtime.o \
time/gmtime_r.o \
time/mktime.o \
timespec.o \
time/strftime.o \
time/timegm.o \
ungetc.o \
vfscanf.o \
vsscanf.o \
wchar/mbrlen.o \
wchar/mbrtowc.o \
wchar/mbsrtowcs.o \
wchar/wcrtomb.o \
wchar/wcscat.o \
wchar/wcschrnul.o \
wchar/wcschr.o \
wchar/wcscmp.o \
wchar/wcscpy.o \
wchar/wcscspn.o \
wchar/wcslen.o \
wchar/wcsncat.o \
wchar/wcsncpy.o \
wchar/wcsrchr.o \
wchar/wcsrtombs.o \
wchar/wcsspn.o \
wchar/wcstok.o \
wcstombs.o \
wctomb.o \
wctype.o \

HOSTEDOBJS=\
access.o \
alarmns.o \
alarm.o \
arpa/inet/inet_addr.o \
arpa/inet/inet_ntoa.o \
arpa/inet/inet_ntop.o \
arpa/inet/inet_pton.o \
basename.o \
calltrace.o \
canonicalize_file_name_at.o \
canonicalize_file_name.o \
chdir.o \
chmod.o \
chown.o \
chroot.o \
close.o \
confstr.o \
$(CPUDIR)/calltrace.o \
$(CPUDIR)/fork.o \
$(CPUDIR)/setjmp.o \
$(CPUDIR)/signal.o \
$(CPUDIR)/syscall.o \
creat.o \
dirent/fddir-sortix.o \
dirent/scandir.o \
dirname.o \
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
fchdirat.o \
fchdir.o \
fchmodat.o \
fchmod.o \
fchownat.o \
fchown.o \
fchrootat.o \
fchroot.o \
fcloseall.o \
fcntl.o \
fdio.o \
fgetpos.o \
fileno.o \
fork.o \
fpipe.o \
freopen.o \
fsetpos.o \
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
gethostname.o \
getlogin.o \
getlogin_r.o \
getpagesize.o \
getpgid.o \
getpid.o \
getppid.o \
gettermmode.o \
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
mkpartition.o \
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
preadv.o \
print.o \
psignal.o \
putc.o \
pwent.o \
pwritev.o \
raise.o \
rand.o \
readdirents.o \
readlinkat.o \
readlink.o \
read.o \
readv.o \
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
setlocale.o \
setpgid.o \
settermmode.o \
setuid.o \
sfork.o \
sigaction.o \
SIG_DFL.o \
SIG_ERR.o \
SIG_IGN.o \
signal.o \
sigprocmask.o \
sleep.o \
stat.o \
stdio.o \
sysconf.o \
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
sys/time/gettimeofday.o \
tcgetpgrp.o \
tcgetwinsize.o \
tcsetpgrp.o \
tfork.o \
time/clock_getres.o \
time/clock_gettime.o \
time/clock_gettimeres.o \
time/clock_nanosleep.o \
time/clock.o \
time/clock_settime.o \
time/clock_settimeres.o \
time/ctime.o \
time/ctime_r.o \
time/difftime.o \
time/localtime.o \
time/localtime_r.o \
time/nanosleep.o \
time/timens.o \
time/time.o \
time/timer_create.o \
time/timer_delete.o \
time/timer_getoverrun.o \
time/timer_gettime.o \
time/timer_settime.o \
time/times.o \
tmpfile.o \
tmpnam.o \
truncateat.o \
truncate.o \
ttyname.o \
umask.o \
unlinkat.o \
unlink.o \
usleep.o \
utimensat.o \
utimens.o \
utime.o \
vscanf.o \
wait.o \
waitpid.o \
write.o \
writev.o \

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

