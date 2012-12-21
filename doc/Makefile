include ../compiler.mak
include ../version.mak
include ../dirs.mak

DOCUMENTS:=\
cross-development \
obsolete-stuff \
user-guide \
welcome \

all: $(BINARIES)

.PHONY: all install clean

install: all
	mkdir -p $(DESTDIR)$(DOCDIR)
	cp -t $(DESTDIR)$(DOCDIR) $(DOCUMENTS)

clean:
