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
	@echo Try: $(MAKE) $(MAKEFLAGS) HOST=$(UNAME_MACHINE)-sortix
	@exit 1
  endif
endif

# Determine whether a function is unset or is a Make default.
is_unset_or_default = $(filter undefined default,$(origin $(1)))

# Provide deprecated CPU variable so makefiles can select CPU-specific dirs.
ifeq ($(HOST),i386-sortix)
    MACHINE:=i386
    CPU:=x86
endif
ifeq ($(HOST),i486-sortix)
    MACHINE:=i486
    CPU:=x86
endif
ifeq ($(HOST),i586-sortix)
    MACHINE:=i586
    CPU:=x86
endif
ifeq ($(HOST),i686-sortix)
    MACHINE:=i686
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
ifndef CC_FOR_BUILD
  CC_FOR_BUILD:=$(BUILD_TOOL_PREFIX)gcc
endif
ifndef CXX_FOR_BUILD
  CXX_FOR_BUILD:=$(BUILD_TOOL_PREFIX)g++
endif
ifndef AR_FOR_BUILD
  AR_FOR_BUILD:=$(BUILD_TOOL_PREFIX)ar
endif
ifndef AS_FOR_BUILD
  AS_FOR_BUILD:=$(BUILD_TOOL_PREFIX)as
endif
ifndef LD_FOR_BUILD
  LD_FOR_BUILD:=$(BUILD_TOOL_PREFIX)ld
endif
ifndef OBJCOPY_FOR_BUILD
  OBJCOPY_FOR_BUILD:=$(BUILD_TOOL_PREFIX)objcopy
endif

# Determine the names of the tools that target the host platform.
ifneq ($(call is_unset_or_default,CC),)
  CC:=$(HOST_TOOL_PREFIX)gcc
endif
ifneq ($(call is_unset_or_default,CXX),)
  CXX:=$(HOST_TOOL_PREFIX)g++
endif
ifneq ($(call is_unset_or_default,AR),)
  AR:=$(HOST_TOOL_PREFIX)ar
endif
ifneq ($(call is_unset_or_default,AS),)
  AS:=$(HOST_TOOL_PREFIX)as
endif
ifneq ($(call is_unset_or_default,LD),)
  LD:=$(HOST_TOOL_PREFIX)ld
endif
ifneq ($(call is_unset_or_default,OBJCOPY),)
  OBJCOPY:=$(HOST_TOOL_PREFIX)objcopy
endif
ifdef SYSROOT
  CC:=$(CC) --sysroot="$(SYSROOT)"
  CXX:=$(CXX) --sysroot="$(SYSROOT)"
  LD:=$(LD) --sysroot="$(SYSROOT)"
endif

# Determine default optimization level.
DEFAULT_GENERIC_OPTLEVEL_BASE:=-O2 -g
DEFAULT_OPTLEVEL_FOR_BUILD:=$(DEFAULT_GENERIC_OPTLEVEL_BASE)
ifeq ($(BUILD_IS_SORTIX),1)
  DEFAULT_OPTLEVEL_FOR_BUILD+=
endif
DEFAULT_OPTLEVEL:=$(DEFAULT_GENERIC_OPTLEVEL_BASE)
ifeq ($(HOST_IS_SORTIX),1)
  DEFAULT_OPTLEVEL+=
endif
