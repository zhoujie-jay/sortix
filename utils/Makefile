SOFTWARE_MEANT_FOR_SORTIX=1
include ../compiler.mak
include ../version.mak
include ../dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES:=\
basename \
calc \
cat \
chmod \
chroot \
chvideomode \
clear \
colormake \
column \
command-not-found \
cp \
date \
dirname \
du \
echo \
editor \
env \
expr \
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
pager \
pwd \
rm \
rmdir \
sh \
sort \
tail \
time \
tr \
true \
type \
uname \
uniq \
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

clean:
	rm -f $(BINARIES) *.o

