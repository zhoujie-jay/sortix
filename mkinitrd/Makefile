SORTIXKERNEL=../sortix
LIBC=../libc

CPPFLAGS=-I../sortix/include
CXXFLAGS=-Wall

BINARIES=mkinitrd initrdfs

all: $(BINARIES)

%: %.cpp crc32.cpp $(LIBC)/ioleast.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< crc32.cpp $(LIBC)/ioleast.cpp -o $@

clean:
	rm -f $(BINARIES)

install:

