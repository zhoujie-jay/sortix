include ../compiler.mak
include ../version.mak
include ../dirs.mak

ifndef OPTLEVEL
  OPTLEVEL:=-O2
endif

ifndef CXXFLAGS
  CXXFLAGS:=$(OPTLEVEL)
endif

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
	$(HOSTAR) rcs $@ $(CLIENT_OBJS)

server: server/dispd

server/dispd: $(SERVER_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SERVER_OBJS) -o $@

clean:
	rm -f $(CLIENT_OBJS) $(SERVER_OBJS)
	rm -f $(BINS)
	rm -f *.o client/*.o server/*.o

%.o: %.cpp
	$(HOSTCXX) -c $< -o $@ $(CPPFLAGS) $(CXXFLAGS)

# Installation into sysroot
install: install-headers install-client install-server

install-include-dirs: headers
	mkdir -p $(INSTALLINCLUDEDIR)
	for D in $$(find include/ -type d | sed 's/include\/*//'); do \
	  mkdir -p $(INSTALLINCLUDEDIR)/$$D || exit $?; \
	done

install-headers: install-include-dirs headers
	for SRC in $$(find include/ -type f); do \
	  DEST=`echo $$SRC | sed 's/include\///'`; \
	  cp -P $$SRC $(INSTALLINCLUDEDIR)/$$DEST || exit $?; \
	done

install-client-dirs:
	mkdir -p $(INSTALLLIBDIR)
	mkdir -p $(INSTALLLIBDIR)/$(HOST)

install-client: install-client-dirs
	cp -P client/libdispd.a $(INSTALLLIBDIR)/$(HOST)

install-server-dirs:
	mkdir -p $(INSTALLBINDIR)/$(HOST)

install-server: install-server-dirs
	install server/dispd $(INSTALLBINDIR)/$(HOST)
