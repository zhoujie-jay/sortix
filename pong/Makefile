# Set up variables such that we can easily cross-compile.
OSROOT=..
include ../crosscompilemakefile.mak

all: pong

pong: pong.o
	$(LD) $(LDFLAGS) pong.o -o pong $(LIBS)

pong.o: pong.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -O2 -c pong.cpp -o pong.o

clean:
	rm -f pong.o pong

