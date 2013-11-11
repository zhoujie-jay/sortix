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
chroot \
chvideomode \
clear \
colormake \
column \
command-not-found \
cp \
date \
du \
echo \
editor \
false \
find \
head \
help \
init \
install-file \
kernelinfo \
kill \
ln \
ls \
memstat \
mkdir \
mv \
mxsh \
pager \
pwd \
rm \
rmdir \
sh \
tail \
time \
true \
type \
uname \
uptime \
wc \
which \

INSTALLBINARIES:=$(addprefix $(DESTDIR)$(BINDIR)/,$(BINARIES))

all: $(BINARIES)

.PHONY: all install uninstall clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES) $(DESTDIR)$(BINDIR)
	install install-file $(DESTDIR)$(BINDIR)/install

uninstall:
	rm -f $(INSTALLBINARIES) $(DESTDIR)$(BINDIR)/install

%: %.cpp
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< -o $@

sh: mxsh
	cp $< $@

clean:
	rm -f $(BINARIES) *.o

