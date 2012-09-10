include ../compiler.mak
include ../version.mak
include ../dirs.mak

ifndef OPTLEVEL
  OPTLEVEL=-O2
endif

ifndef CXXFLAGS
  CXXFLAGS:=$(OPTLEVEL)
endif

CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti

BINARIES:=\
init \
calc \
cat \
cp \
echo \
rm \
sh \
mxsh \
clear \
ls \
pwd \
help \
uptime \
memstat \
chvideomode \
uname \
kernelinfo \
editor \
kill \
column \
pager \
head \
tail \
type \

all: $(BINARIES)

.PHONY: all install uninstall clean

install: all
	mkdir -p $(INSTALLBINDIR)/$(HOST)
	install $(BINARIES) $(INSTALLBINDIR)/$(HOST)

uninstall:
	for FILE in $(INSTALLBINDIR); do \
		rm -f $(INSTALLBINDIR)/$(HOST)/$$FILE; \
	done

%: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

sh: mxsh
	cp $< $@

clean:
	rm -f $(BINARIES) *.o

