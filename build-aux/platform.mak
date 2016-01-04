UNAME_OS:=$(shell uname -o)
UNAME_PLATFORM:=$(shell uname -i)

# Detect the platform that the software is being built on.
ifndef BUILD

  # If the software is being built on Sortix.
  ifeq ($(UNAME_OS),Sortix)
    ifeq ($(MAKEFILE_NOT_MEANT_FOR_SORTIX), 1)
makefile_not_meant_for_sortix:
	@echo This makefile isn\'t meant to work under Sortix
	@exit 1
    endif
    ifeq ($(UNAME_PLATFORM),i386)
      BUILD:=i486-sortix
    endif
    ifeq ($(UNAME_PLATFORM),x86_64)
      BUILD:=x86_64-sortix
    endif

  # If the software is not built on Sortix.
  else
    ifeq ($(MEANT_FOR_SORTIX), 1)
makefile_meant_for_sortix:
	@echo This makefile isn\'t meant to work under $(UNAME_OS)
	@exit 1
    endif
  endif

  ifeq ($(UNAME_OS),GNU/Linux)
    ifeq ($(UNAME_PLATFORM),i386)
      BUILD:=i386-linux-gnu
    endif
    ifeq ($(UNAME_PLATFORM),x86_64)
      BUILD:=x86_64-linux-gnu
    endif
  endif

endif

# Determine the platform the software will run on.
HOST?=$(BUILD)
HOST_MACHINE:=$(shell expr x$(HOST) : 'x\([^-]*\).*')

# Determine the platform the software will target.
TARGET?=$(HOST)

BUILD_IS_SORTIX:=$(if $(shell echo $(BUILD) | grep sortix$),1,0)
HOST_IS_SORTIX:=$(if $(shell echo $(HOST) | grep sortix$),1,0)
TARGET_IS_SORTIX:=$(if $(shell echo $(TARGET) | grep sortix$),1,0)
