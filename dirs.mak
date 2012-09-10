ifndef SYSROOT
  SYSROOT:=/
endif

# The locations from where the program will be invoked/run on the host machine.
# For instance, on Sortix programs are normally run from the /bin union
# directory, even though they are actually installed somewhere else.
ifndef RUNPREFIXNAME
  RUNPREFIXNAME:=/
endif
RUNPREFIXNAME:=$(shell echo "$(RUNPREFIXNAME)" | sed 's/\/\/*/\//g')

ifndef RUNPREFIX
  RUNPREFIX:=/$(RUNPREFIXNAME)
endif
RUNPREFIX:=$(shell echo "$(RUNPREFIX)" | sed 's/\/\/*/\//g')

ifndef RUNBINDIR
  RUNBINDIR:=$(RUNPREFIX)/bin
endif
RUNBINDIR:=$(shell echo "$(RUNBINDIR)" | sed 's/\/\/*/\//g')

ifndef RUNLIBDIR
  RUNLIBDIR:=$(RUNPREFIX)/lib
endif
RUNLIBDIR:=$(shell echo "$(RUNLIBDIR)" | sed 's/\/\/*/\//g')

ifndef RUNINCLUDEDIR
  RUNINCLUDEDIR:=$(RUNPREFIX)/include
endif
RUNINCLUDEDIR:=$(shell echo "$(RUNINCLUDEDIR)" | sed 's/\/\/*/\//g')

ifndef RUNBOOTDIR
  RUNBOOTDIR:=$(RUNPREFIX)/boot
endif
RUNBOOTDIR:=$(shell echo "$(RUNBOOTDIR)" | sed 's/\/\/*/\//g')

# Where the program will be really installed on the host machine. For instance,
# on Sortix programs are run from the /bin union directory, but they are
# installed somewhere else, these variables tells where. Note that programs must
# not depend on being installed here, they should detect the proper directories
# at (preferably) runtime, or during the install configuration step when
# installed through package management.
ifndef PREFIXNAME
  PREFIXNAME:=/data/local
endif
PREFIXNAME:=$(shell echo "$(PREFIXNAME)" | sed 's/\/\/*/\//g')

ifndef PREFIX
  PREFIX:=$(SYSROOT)/$(PREFIXNAME)
endif
PREFIX:=$(shell echo "$(PREFIX)" | sed 's/\/\/*/\//g')

ifndef BINDIR
  BINDIR:=$(PREFIX)/bin
endif
BINDIR:=$(shell echo "$(BINDIR)" | sed 's/\/\/*/\//g')

ifndef LIBDIR
  LIBDIR:=$(PREFIX)/lib
endif
LIBDIR:=$(shell echo "$(LIBDIR)" | sed 's/\/\/*/\//g')

ifndef INCLUDEDIR
  INCLUDEDIR:=$(PREFIX)/include
endif
INCLUDEDIR:=$(shell echo "$(INCLUDEDIR)" | sed 's/\/\/*/\//g')

ifndef BOOTDIR
  BOOTDIR:=$(PREFIX)/boot
endif
BOOTDIR:=$(shell echo "$(BOOTDIR)" | sed 's/\/\/*/\//g')

# Where the program will be installed on the build machine. This is mostly
# useful if cross compiling or producing a software distribution image. By
# default programs will be installed in the same location as the host machine.
ifdef INSTALLPREFIX
  INSTALLPREFIX_WAS_SPECIFIED=1
endif
ifndef INSTALLPREFIX
    INSTALLPREFIX:=$(PREFIX)
endif
INSTALLPREFIX:=$(shell echo "$(INSTALLPREFIX)" | sed 's/\/\/*/\//g')

ifndef INSTALLBINDIR
  ifdef INSTALLPREFIX_WAS_SPECIFIED
    INSTALLBINDIR:=$(INSTALLPREFIX)/bin
  else
    INSTALLBINDIR:=$(BINDIR)
  endif
endif
INSTALLBINDIR:=$(shell echo "$(INSTALLBINDIR)" | sed 's/\/\/*/\//g')

ifndef INSTALLLIBDIR
  ifdef INSTALLPREFIX_WAS_SPECIFIED
    INSTALLLIBDIR:=$(INSTALLPREFIX)/lib
  else
    INSTALLLIBDIR:=$(LIBDIR)
  endif
endif
INSTALLLIBDIR:=$(shell echo "$(INSTALLLIBDIR)" | sed 's/\/\/*/\//g')

ifndef INSTALLINCLUDEDIR
  ifdef INSTALLPREFIX_WAS_SPECIFIED
    INSTALLINCLUDEDIR:=$(INSTALLPREFIX)/include
  else
    INSTALLINCLUDEDIR:=$(INCLUDEDIR)
  endif
endif
INSTALLINCLUDEDIR:=$(shell echo "$(INSTALLINCLUDEDIR)" | sed 's/\/\/*/\//g')

ifndef INSTALLBOOTDIR
  ifdef INSTALLPREFIX_WAS_SPECIFIED
    INSTALLBOOTDIR:=$(INSTALLPREFIX)/boot
  else
    INSTALLBOOTDIR:=$(BOOTDIR)
  endif
endif
INSTALLBOOTDIR:=$(shell echo "$(INSTALLBOOTDIR)" | sed 's/\/\/*/\//g')
