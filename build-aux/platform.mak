UNAME_OS:=$(shell uname -s | tr '[:upper:]' '[:lower:]' | sed 's,^linux$$,linux-gnu,g')
UNAME_MACHINE:=$(shell uname -m)

# Detect the platform that the software is being built on.
BUILD?=$(UNAME_MACHINE)-$(UNAME_OS)

# Determine the platform the software will run on.
HOST?=$(BUILD)
HOST_MACHINE:=$(shell expr x$(HOST) : 'x\([^-]*\).*')

# Determine the platform the software will target.
TARGET?=$(HOST)

BUILD_IS_SORTIX:=$(if $(shell echo $(BUILD) | grep -- -sortix),1,0)
HOST_IS_SORTIX:=$(if $(shell echo $(HOST) | grep -- -sortix),1,0)
TARGET_IS_SORTIX:=$(if $(shell echo $(TARGET) | grep -- -sortix),1,0)

# If the software is being built on Sortix.
ifeq ($(UNAME_OS),sortix)
  ifeq ($(MAKEFILE_NOT_MEANT_FOR_SORTIX), 1)
makefile_not_meant_for_sortix:
	@echo This makefile isn\'t meant to work under Sortix
	@exit 1
  endif

# If the software is not built on Sortix.
else ifeq ($(MEANT_FOR_SORTIX), 1)
makefile_meant_for_sortix:
	@echo This makefile isn\'t meant to work under $(uname -s)
	@exit 1
endif
