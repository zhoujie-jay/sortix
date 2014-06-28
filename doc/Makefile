include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

DOCUMENTS:=\
cross-compilation-sins \
cross-development \
obsolete-stuff \
porting-guide \
posix-divergence \
user-guide \
welcome \

all: $(BINARIES)

.PHONY: all install clean

install: all
	mkdir -p $(DESTDIR)$(DOCDIR)
	cp -t $(DESTDIR)$(DOCDIR) $(DOCUMENTS)

clean:
