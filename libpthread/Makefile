SOFTWARE_MEANT_FOR_SORTIX=1
include ../compiler.mak
include ../version.mak
include ../dirs.mak

OPTLEVEL?=-O2 -g
CXXFLAGS?=$(OPTLEVEL)
CPPFLAGS?=

CPPFLAGS:=$(CPPFLAGS) -D__is_sortix_libpthread -I include
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

OBJS=\
pthread_condattr_destroy.o \
pthread_condattr_getclock.o \
pthread_condattr_init.o \
pthread_condattr_setclock.o \
pthread_cond_broadcast.o \
pthread_cond_destroy.o \
pthread_cond_init.o \
pthread_cond_signal.o \
pthread_cond_timedwait.o \
pthread_cond_wait.o \
pthread_equal.o \
pthread_exit.o \
pthread_initialize.o \
pthread_key_create.o \
pthread_key_delete.o \
pthread_mutexattr_destroy.o \
pthread_mutexattr_gettype.o \
pthread_mutexattr_init.o \
pthread_mutexattr_settype.o \
pthread_mutex_destroy.o \
pthread_mutex_init.o \
pthread_mutex_lock.o \
pthread_mutex_trylock.o \
pthread_mutex_unlock.o \
pthread_rwlock_destroy.o \
pthread_rwlock_init.o \
pthread_rwlock_rdlock.o \
pthread_rwlock_tryrdlock.o \
pthread_rwlock_trywrlock.o \
pthread_rwlock_unlock.o \
pthread_rwlock_wrlock.o \
pthread_self.o \

BINS:=libpthread.a

# Main build rules.
all: $(BINS)

.PHONY: headers clean install install-headers libs install-libs

headers:

libs: $(BINS)

libpthread.a: $(OBJS)
	$(HOSTAR) rcs $@ $(OBJS)

%.o: %.c++
	$(HOSTCXX) -std=gnu++11 -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

clean:
	rm -f $(BINS) $(OBJS) *.o

# Installation into sysroot.
install: install-headers install-libs

install-headers:  headers
	cp -RTv include $(DESTDIR)$(INCLUDEDIR)

install-libs:
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp -P libpthread.a $(DESTDIR)$(LIBDIR)
