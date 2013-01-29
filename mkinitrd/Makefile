SORTIXKERNEL=../sortix
LIBC=../libc

CPPFLAGS=-I../sortix/include
CXXFLAGS=-g -Wall -Wextra

BINARIES=mkinitrd initrdfs

all: $(BINARIES)

%: %.cpp crc32.cpp rules.cpp $(LIBC)/ioleast.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< crc32.cpp rules.cpp $(LIBC)/ioleast.cpp -o $@

clean:
	rm -f $(BINARIES)

install:

