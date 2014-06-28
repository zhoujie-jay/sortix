SOFTWARE_MEANT_FOR_SORTIX=1
include ../build-aux/platform.mak
include ../build-aux/compiler.mak
include ../build-aux/version.mak
include ../build-aux/dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

CPPFLAGS:=$(CPPFLAGS) -DVERSIONSTR=\"$(VERSION)\"
CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES_EXCEPT_INSTALL:=\
basename \
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
sleep \
sort \
sortix-sh \
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

BINARIES=\
$(BINARIES_EXCEPT_INSTALL) \
xinstall

all: $(BINARIES)

.PHONY: all install clean

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	install $(BINARIES_EXCEPT_INSTALL) $(DESTDIR)$(BINDIR)
	install xinstall $(DESTDIR)$(BINDIR)/install

%: %.cpp
	$(CXX) -std=gnu++11 $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(BINARIES) *.o
