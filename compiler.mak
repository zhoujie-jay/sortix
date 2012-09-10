ifndef BITS
  BITS:=$(shell getconf LONG_BIT)
endif

ifndef HOST
  ifeq ($(BITS),64)
    HOST:=x86_64-sortix
  else
    HOST:=i486-sortix
  endif
  MFLAGS:=$(MFLAGS) HOST=$(HOST)
endif

ifeq ($(HOST),i486-sortix)
    CPU:=x86
endif
ifeq ($(HOST),x86_64-sortix)
    CPU:=x64
endif

ifndef BUILDCC
  BUILDCC:=gcc
endif
ifndef BUILDCXX
  BUILDCXX:=g++
endif
ifndef BUILDAR
  BUILDAR:=ar
endif
ifndef BUILDAS
  BUILDAS:=as
endif
ifndef BUILDLD
  BUILDAS:=ld
endif
ifndef BUILDOBJCOPY
  BUILDOBJCOPY:=objcopy
endif

ifndef HOSTCC
  HOSTCC:=$(HOST)-gcc
endif
ifndef HOSTCXX
  HOSTCXX:=$(HOST)-g++
endif
ifndef HOSTAR
  HOSTAR:=$(HOST)-ar
endif
ifndef HOSTAS
  HOSTAS:=$(HOST)-as
endif
ifndef HOSTLD
  HOSTLD:=$(HOST)-ld
endif
ifndef HOSTOBJCOPY
  HOSTOBJCOPY:=$(HOST)-objcopy
endif

CC:=$(HOSTCC)
CXX:=$(HOSTCXX)
AR=:$(HOSTAR)
AS:=$(HOSTAS)
LD:=$(HOSTLD)
OBJCOPY:=$(HOSTOBJCOPY)

