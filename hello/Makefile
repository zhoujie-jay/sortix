# Set up variables such that we can easily cross-compile.
OSROOT=..
include ../crosscompilemakefile.mak

all: hello

hello: hello.o
	$(LD) $(LDFLAGS) hello.o -o hello $(LIBS)

hello.o: hello.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O2 -c hello.cpp -o hello.o

clean:
	rm -f hello.o hello

