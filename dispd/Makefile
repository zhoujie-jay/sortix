SOFTWARE_MEANT_FOR_SORTIX=1
include ../compiler.mak
include ../version.mak
include ../dirs.mak

OPTLEVEL?=$(DEFAULT_OPTLEVEL)
CXXFLAGS?=$(OPTLEVEL)

CXXFLAGS:=$(CXXFLAGS) -Wall -Wextra -fno-exceptions -fno-rtti
CPPFLAGS:=$(CPPFLAGS) -I include

CLIENT_OBJS:=\
client/library.o \
client/session.o \
client/window.o \

SERVER_OBJS:=\
server/dispd.o \

BINS:=server/dispd client/libdispd.a

all: $(BINS)

.PHONY: all headers client server clean install install-include-dirs \
        install-headers install-client-dirs install-client install-server-dirs \
        install-server

headers:

client: client/libdispd.a

client/libdispd.a: $(CLIENT_OBJS)
	$(AR) rcs $@ $(CLIENT_OBJS)

server: server/dispd

server/dispd: $(SERVER_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SERVER_OBJS) -o $@

clean:
	rm -f $(CLIENT_OBJS) $(SERVER_OBJS)
	rm -f $(BINS)
	rm -f *.o client/*.o server/*.o

%.o: %.cpp
	$(CXX) -std=gnu++11 -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

# Installation into sysroot
install: install-headers install-client install-server

install-include-dirs: headers
	mkdir -p $(DESTDIR)$(INCLUDEDIR)

install-headers: install-include-dirs headers
	cp -RTv include $(DESTDIR)$(INCLUDEDIR)

install-client-dirs:
	mkdir -p $(DESTDIR)$(LIBDIR)

install-client: install-client-dirs client
	cp -P client/libdispd.a $(DESTDIR)$(LIBDIR)

install-server-dirs:
	mkdir -p $(DESTDIR)$(BINDIR)

install-server: install-server-dirs server
	install server/dispd $(DESTDIR)$(BINDIR)
