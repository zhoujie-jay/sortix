# Set up variables such that we can easily cross-compile.
OSROOT=..
include ../crosscompilemakefile.mak

INITRDDIR:=../initrd
LOCALBINARIES:=\
init \
cat \
sh \
mxsh \
clear \
ls \
help \
uname \

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

