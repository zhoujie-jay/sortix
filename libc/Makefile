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
assert/_assert.o \
aux/c++.o \
aux/op-new.o \
ctype/ctype.o \
dirent/alphasort.o \
dirent/dir.o \
dirent/versionsort.o \
errno/errno.o \
inttypes/imaxabs.o \
inttypes/imaxdiv.o \
inttypes/strtoimax.o \
inttypes/strtoumax.o \
inttypes/wcstoimax.o \
inttypes/wcstoumax.o \
signal/sigaddset.o \
signal/sigandset.o \
signal/sigdelset.o \
signal/sigemptyset.o \
signal/sigfillset.o \
signal/sigisemptyset.o \
signal/sigismember.o \
signal/signotset.o \
signal/sigorset.o \
stdio/clearerr.o \
stdio/dprintf.o \
stdio/fbufsize.o \
stdio/fclose.o \
stdio/fdeletefile.o \
stdio/feof.o \
stdio/ferror.o \
stdio/fflush.o \
stdio/fflush_stop_reading.o \
stdio/fflush_stop_writing.o \
stdio/fgetc.o \
stdio/fgets.o \
stdio/flbf.o \
stdio/flockfile.o \
stdio/flushlbf.o \
stdio/fnewfile.o \
stdio/format.o \
stdio/fpending.o \
stdio/fpurge.o \
stdio/fputc.o \
stdio/fputs.o \
stdio/freadable.o \
stdio/freading.o \
stdio/fread.o \
stdio/fregister.o \
stdio/fresetfile.o \
stdio/fscanf.o \
stdio/fseek.o \
stdio/fseeko.o \
stdio/fsetdefaultbuf.o \
stdio/fseterr.o \
stdio/fsetlocking.o \
stdio/fshutdown.o \
stdio/ftell.o \
stdio/ftello.o \
stdio/ftrylockfile.o \
stdio/funlockfile.o \
stdio/fwritable.o \
stdio/fwrite.o \
stdio/fwriting.o \
stdio/getdelim.o \
stdio/getline.o \
stdio/rewind.o \
stdio/setbuf.o \
stdio/setvbuf.o \
stdio/snprintf.o \
stdio/sprintf.o \
stdio/sscanf.o \
stdio/ungetc.o \
stdio/vdprintf.o \
stdio/vfscanf.o \
stdio/vsnprintf.o \
stdio/vsprintf.o \
stdio/vsscanf.o \
stdlib/abort.o \
stdlib/abs.o \
stdlib/atof.o \
stdlib/atoi.o \
stdlib/atoll.o \
stdlib/atol.o \
stdlib/bsearch.o \
stdlib/calloc.o \
stdlib/div.o \
stdlib/heap.o \
stdlib/labs.o \
stdlib/ldiv.o \
stdlib/llabs.o \
stdlib/lldiv.o \
stdlib/mblen.o \
stdlib/mbstowcs.o \
stdlib/mbtowc.o \
stdlib/qsort.o \
stdlib/strtod.o \
stdlib/strtof.o \
stdlib/strtold.o \
stdlib/strtoll.o \
stdlib/strtol.o \
stdlib/strtoull.o \
stdlib/strtoul.o \
stdlib/wcstombs.o \
stdlib/wctomb.o \
string/ffsll.o \
string/ffsl.o \
string/ffs.o \
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
string/strcoll_l.o \
string/strcoll.o \
string/strcpy.o \
string/strcspn.o \
string/strdup.o \
string/strerror_l.o \
string/strerror.o \
string/strerror_r.o \
string/strlcat.o \
string/strlcpy.o \
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
string/strverscmp.o \
string/strxfrm_l.o \
string/strxfrm.o \
time/asctime.o \
time/asctime_r.o \
time/gmtime.o \
time/gmtime_r.o \
time/mktime.o \
timespec/timespec.o \
time/strftime_l.o \
time/strftime.o \
time/timegm.o \
wchar/mbrlen.o \
wchar/mbrtowc.o \
wchar/mbsrtowcs.o \
wchar/wcrtomb.o \
wchar/wcscat.o \
wchar/wcschrnul.o \
wchar/wcschr.o \
wchar/wcscmp.o \
wchar/wcscoll.o \
wchar/wcscpy.o \
wchar/wcscspn.o \
wchar/wcslen.o \
wchar/wcsncat.o \
wchar/wcsncpy.o \
wchar/wcsrchr.o \
wchar/wcsrtombs.o \
wchar/wcsspn.o \
wchar/wcstok.o \
wchar/wcstoll.o \
wchar/wcstol.o \
wchar/wcstoull.o \
wchar/wcstoul.o \
wctype/wctype.o \

HOSTEDOBJS=\
arpa/inet/inet_addr.o \
arpa/inet/inet_ntoa.o \
arpa/inet/inet_ntop.o \
arpa/inet/inet_pton.o \
calltrace/calltrace.o \
$(CPUDIR)/calltrace.o \
$(CPUDIR)/fork.o \
$(CPUDIR)/setjmp.o \
$(CPUDIR)/signal.o \
$(CPUDIR)/syscall.o \
dirent/fddir-sortix.o \
dirent/scandir.o \
dlfcn/dlfcn.o \
error/gnu_error.o \
fcntl/creat.o \
fcntl/fcntl.o \
fcntl/openat.o \
fcntl/open.o \
fsmarshall/fsm_bootstraprootfd.o \
fsmarshall/fsm_closechannel.o \
fsmarshall/fsm_closeserver.o \
fsmarshall/fsm_fsbind.o \
fsmarshall/fsm_listen.o \
fsmarshall/fsm_mkserver.o \
fsmarshall/fsm_recv.o \
fsmarshall/fsm_send.o \
getopt/getopt_long.o \
getopt/getopt.o \
grp/grent.o \
init/init.o \
libgen/basename.o \
libgen/dirname.o \
locale/localeconv.o \
locale/setlocale.o \
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
poll/poll.o \
poll/ppoll.o \
pwd/pwent.o \
signal/kill.o \
signal/killpg.o \
signal/psignal.o \
signal/raise.o \
signal/sigaction.o \
signal/SIG_DFL.o \
signal/SIG_ERR.o \
signal/SIG_IGN.o \
signal/signal.o \
signal/sigprocmask.o \
stdio/fcloseall.o \
stdio/fdio.o \
stdio/fgetpos.o \
stdio/fileno.o \
stdio/fpipe.o \
stdio/fprintf.o \
stdio/freopen.o \
stdio/fsetpos.o \
stdio/getc.o \
stdio/perror.o \
stdio/popen.o \
stdio/printf.o \
stdio/putc.o \
stdio/removeat.o \
stdio/remove.o \
stdio/renameat.o \
stdio/rename.o \
stdio/scanf.o \
stdio/stdio.o \
stdio/tmpfile.o \
stdio/tmpnam.o \
stdio/vfprintf.o \
stdio/vprintf.o \
stdio/vscanf.o \
stdlib/canonicalize_file_name_at.o \
stdlib/canonicalize_file_name.o \
stdlib/env.o \
stdlib/_Exit.o \
stdlib/exit.o \
stdlib/mktemp.o \
stdlib/on_exit.o \
stdlib/rand.o \
stdlib/realpath.o \
stdlib/system.o \
sys/display/dispmsg_issue.o \
sys/ioctl/ioctl.o \
sys/kernelinfo/kernelinfo.o \
sys/mman/mmap.o \
sys/mman/mprotect.o \
sys/mman/munmap.o \
sys/readdirents/readdirents.o \
sys/select/select.o \
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
sys/stat/chmod.o \
sys/stat/fchmodat.o \
sys/stat/fchmod.o \
sys/stat/fstatat.o \
sys/stat/fstat.o \
sys/stat/futimens.o \
sys/stat/lstat.o \
sys/stat/mkdirat.o \
sys/stat/mkdir.o \
sys/stat/stat.o \
sys/stat/umask.o \
sys/stat/utimensat.o \
sys/stat/utimens.o \
sys/termmode/gettermmode.o \
sys/termmode/settermmode.o \
sys/time/gettimeofday.o \
sys/uio/preadv.o \
sys/uio/pwritev.o \
sys/uio/readv.o \
sys/uio/writev.o \
sys/wait/wait.o \
sys/wait/waitpid.o \
termios/tcgetwinsize.o \
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
unistd/access.o \
unistd/alarmns.o \
unistd/alarm.o \
unistd/chdir.o \
unistd/chown.o \
unistd/chroot.o \
unistd/close.o \
unistd/confstr.o \
unistd/dup2.o \
unistd/dup.o \
unistd/execle.o \
unistd/execl.o \
unistd/execlp.o \
unistd/execve.o \
unistd/execv.o \
unistd/execvpe.o \
unistd/execvp.o \
unistd/_exit.o \
unistd/faccessat.o \
unistd/fchdirat.o \
unistd/fchdir.o \
unistd/fchownat.o \
unistd/fchown.o \
unistd/fchrootat.o \
unistd/fchroot.o \
unistd/fork.o \
unistd/fsync.o \
unistd/ftruncate.o \
unistd/getcwd.o \
unistd/getdtablesize.o \
unistd/getegid.o \
unistd/geteuid.o \
unistd/getgid.o \
unistd/gethostname.o \
unistd/getlogin.o \
unistd/getlogin_r.o \
unistd/getpagesize.o \
unistd/getpgid.o \
unistd/getpid.o \
unistd/getppid.o \
unistd/getuid.o \
unistd/ioleast.o \
unistd/isatty.o \
unistd/lchown.o \
unistd/linkat.o \
unistd/link.o \
unistd/lseek.o \
unistd/memstat.o \
unistd/mkpartition.o \
unistd/pathconf.o \
unistd/pipe.o \
unistd/pread.o \
unistd/pwrite.o \
unistd/readlinkat.o \
unistd/readlink.o \
unistd/read.o \
unistd/rmdir.o \
unistd/sbrk.o \
unistd/setegid.o \
unistd/seteuid.o \
unistd/setgid.o \
unistd/setpgid.o \
unistd/setuid.o \
unistd/sfork.o \
unistd/sleep.o \
unistd/sysconf.o \
unistd/tcgetpgrp.o \
unistd/tcsetpgrp.o \
unistd/tfork.o \
unistd/truncateat.o \
unistd/truncate.o \
unistd/ttyname.o \
unistd/unlinkat.o \
unistd/unlink.o \
unistd/usleep.o \
unistd/write.o \
utime/utime.o \

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

BINS=libc.a libg.a libpthread.a libstdc++.a $(CRTOBJ)
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

