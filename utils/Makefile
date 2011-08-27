# Set up variables such that we can easily cross-compile.
OSROOT=..
include ../crosscompilemakefile.mak

BINARIES:=\
init \
cat \
sh \
mxsh \
clear \
ls \
help \
uname \

all: $(BINARIES)

%: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O2 -c $< -o $@.o
	$(LD) $(LDFLAGS) $@.o -o $@ $(LIBS)

sh: mxsh
	cp mxsh sh

clean:
	rm -f $(BINARIES) *.o

