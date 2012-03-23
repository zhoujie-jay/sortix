SORTIXKERNEL=../sortix
LIBMAXSI=../libmaxsi

CPPFLAGS=-I../sortix/include
CXXFLAGS=-Wall

BINARIES=mkinitrd initrdfs

all: $(BINARIES)

%: %.cpp crc32.cpp $(LIBMAXSI)/ioleast.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< crc32.cpp $(LIBMAXSI)/ioleast.cpp -o $@

clean:
	rm -f $(BINARIES)

install:

