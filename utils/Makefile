SOFTWARE_MEANT_FOR_SORTIX=1
include ../compiler.mak
include ../version.mak
include ../dirs.mak

ifndef OPTLEVEL
  OPTLEVEL=-g -O2
endif

ifndef CXXFLAGS
  CXXFLAGS:=$(OPTLEVEL)
endif

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES:=\
calc \
cat \
chvideomode \
clear \
column \
cp \
echo \
editor \
head \
help \
init \
kernelinfo \
kill \
ln \
ls \
memstat \
mkdir \
mxsh \
pager \
pwd \
rm \
rmdir \
sh \
tail \
type \
uname \
uptime \
which \

INSTALLBINARIES:=$(addprefix $(DESTDIR)$(BINDIR)/,$(BINARIES))

all: $(BINARIES)

.PHONY: all install uninstall clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(INSTALLBINARIES) $(DESTDIR)$(BINDIR)/install

%: %.cpp
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< -o $@

sh: mxsh
	cp $< $@

clean:
	rm -f $(BINARIES) *.o

