# Warn if default target is used and the software shouldn't be built under Sortix.
ifeq ($(BUILD_IS_SORTIX),1)
  ifeq ($(MAKEFILE_NOT_MEANT_FOR_SORTIX), 1)
makefile_not_meant_for_sortix:
	@echo This makefile isn\'t meant to work under Sortix
	@exit 1
  endif
endif

# Warn if default target is used and the software should be built under Sortix.
ifeq ($(BUILD_IS_SORTIX),0)
  ifeq ($(MAKEFILE_MEANT_FOR_SORTIX), 1)
makefile_meant_for_sortix:
	@echo This makefile is meant to work under Sortix, not $(BUILD)
	@exit 1
  endif
endif

# Warn if default target is used and the software shouldn't be built for Sortix.
ifeq ($(HOST_IS_SORTIX),1)
  ifeq ($(SOFTWARE_NOT_MEANT_FOR_SORTIX), 1)
software_not_meant_for_sortix:
	@echo This software isn\'t meant to work under Sortix
	@exit 1
  endif
endif

# Warn if default target is used and the software should be built for Sortix.
ifeq ($(HOST_IS_SORTIX),0)
  ifeq ($(SOFTWARE_MEANT_FOR_SORTIX), 1)
software_meant_for_sortix:
	@echo This software is meant to work under Sortix, not $(HOST)
	@echo Attempt was $(MAKE) $(MAKEFLAGS)
	@echo Try: $(MAKE) $(MAKEFLAGS) HOST=$(UNAME_PLATFORM)-sortix
	@exit 1
  endif
endif

# Provide deprecated CPU variable so makefiles can select CPU-specific dirs.
ifeq ($(HOST),i486-sortix)
    MACHINE:=i486
    CPU:=x86
endif
ifeq ($(HOST),x86_64-sortix)
    MACHINE:=x86_64
    CPU:=x64
endif

# Determine the prefix for build tools.
ifndef BUILD_TOOL_PREFIX
  BUILD_TOOL_PREFIX:=
endif

# Determine the prefix for host tools.
ifndef HOST_TOOL_PREFIX
  ifeq ($(BUILD),$(HOST))
    HOST_TOOL_PREFIX:=$(BUILD_TOOL_PREFIX)
  else
    HOST_TOOL_PREFIX:=$(HOST)-
  endif
endif

# Determine the names of the tools that target the build platform.
ifndef BUILDCC
  BUILDCC:=$(BUILD_TOOL_PREFIX)gcc
endif
ifndef BUILDCXX
  BUILDCXX:=$(BUILD_TOOL_PREFIX)g++
endif
ifndef BUILDAR
  BUILDAR:=$(BUILD_TOOL_PREFIX)ar
endif
ifndef BUILDAS
  BUILDAS:=$(BUILD_TOOL_PREFIX)as
endif
ifndef BUILDLD
  BUILDAS:=$(BUILD_TOOL_PREFIX)ld
endif
ifndef BUILDOBJCOPY
  BUILDOBJCOPY:=$(BUILD_TOOL_PREFIX)objcopy
endif

# Determine the names of the tools that target the host platform.
ifndef HOSTCC
  HOSTCC:=$(HOST_TOOL_PREFIX)gcc
endif
ifndef HOSTCXX
  HOSTCXX:=$(HOST_TOOL_PREFIX)g++
endif
ifndef HOSTAR
  HOSTAR:=$(HOST_TOOL_PREFIX)ar
endif
ifndef HOSTAS
  HOSTAS:=$(HOST_TOOL_PREFIX)as
endif
ifndef HOSTLD
  HOSTLD:=$(HOST_TOOL_PREFIX)ld
endif
ifndef HOSTOBJCOPY
  HOSTOBJCOPY:=$(HOST_TOOL_PREFIX)objcopy
endif
ifdef SYSROOT
  HOSTCC:=$(HOSTCC) --sysroot="$(SYSROOT)"
  HOSTCXX:=$(HOSTCXX) --sysroot="$(SYSROOT)"
  HOSTLD:=$(HOSTLD) --sysroot="$(SYSROOT)"
endif

CC:=$(HOSTCC)
CXX:=$(HOSTCXX)
AR:=$(HOSTAR)
AS:=$(HOSTAS)
LD:=$(HOSTLD)
OBJCOPY:=$(HOSTOBJCOPY)

# Determine default optimization level.
DEFAULT_GENERIC_OPTLEVEL_BASE:=-O2 -g
DEFAULT_BUILD_OPTLEVEL:=$(DEFAULT_GENERIC_OPTLEVEL_BASE)
ifeq ($(BUILD_IS_SORTIX),1)
  DEFAULT_BUILD_OPTLEVEL+=
endif
DEFAULT_HOST_OPTLEVEL:=$(DEFAULT_GENERIC_OPTLEVEL_BASE)
DEFAULT_OPTLEVEL:=$(DEFAULT_GENERIC_OPTLEVEL_BASE)
ifeq ($(HOST_IS_SORTIX),1)
  DEFAULT_HOST_OPTLEVEL+=
  DEFAULT_OPTLEVEL+=
endif
