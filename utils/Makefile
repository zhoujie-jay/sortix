# Set up variables such that we can easily cross-compile.
OSROOT=..
include ../crosscompilemakefile.mak

INITRDDIR:=../initrd
LOCALBINARIES:=\
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
uname \
idle \
editor \
kill \
column \
pager \

BINARIES:=$(addprefix $(INITRDDIR)/,$(LOCALBINARIES))

all: install

install: $(LOCALBINARIES)
	cp $(LOCALBINARIES) $(INITRDDIR)
	rm -f $(LOCALBINARIES)

%: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O2 -c $< -o $@.o
	$(LD) $(LDFLAGS) $@.o -o $@ $(LIBS)

sh: mxsh
	cp $< $@

clean:
	rm -f $(BINARIES) $(LOCALBINARIES) *.o

