#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([mxmpp], [0.1], [sortie@maxsi.org])
AC_CONFIG_SRCDIR([mxmpp.cpp])
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_SYSTEM

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_MINGW32
AC_EXEEXT
AC_PROG_INSTALL

dnl Some things you can only do by looking at the platform name.
case "${host}" in
  powerpc-apple-darwin*)
    AC_DEFINE([__powerpc64__], [1], [this is a 64 bit powerpc])
    darwin=yes
    AC_DEFINE([DARWIN_HOST], [1], [this is a Darwin platform])
    ;;
  *-apple-darwin*)
    darwin=yes
    AC_DEFINE([DARWIN_HOST], [1], [this is a Darwin platform])
    ;;
    dnl Unfortunately, all BSD distributions are not identical, so 
    dnl as tacky as it is to look for the distribution name, we don't
    dnl have much choice. The use of these should be avoid as much as possible.
  *-openbsd*)
    bsd=yes
    openbsd=yes
    AC_DEFINE([OPENBSD_HOST], [1], [this is an OpenBSD platform])
    ;;
  *-freebsd* | *-kfreebsd*)
    bsd=yes
    freebsd=yes
    AC_DEFINE([FREEBSD_HOST], [1], [this is a FreeBSD platform])
    ;;
  *-netbsd*)
    bsd=yes
    netbsd=yes
    AC_DEFINE([NETBSD_HOST], [1], [this is a NetBSD platform])
    ;;
  *-*solaris*)
    solaris=yes
    AC_DEFINE([SOLARIS_HOST], [1], [this is a Solaris platform])
    ;;
  *-*linux*)
    linux=yes
    AC_DEFINE([LINUX_HOST], [1], [this is a Linux platform])
    ;;
  *-cygwin* | *-mingw* | *-pw32*)
    windows=yes
    AC_DEFINE([WIN32_HOST], [1], [this is a Win32 platform])
    ;;
  *64-*-*bsd*)
    bsd_os=bsd 
    AC_DEFINE([WORDSIZE], [64], [this is a 64 platform])
    ;;
  *-*amigaos*)
    amigaos4=yes
    AC_DEFINE([AMIGAOS4_HOST], [1], [this is an AmigaOS4 platform])
    ;;
  *-*haiku*)
    haiku=yes
    AC_DEFINE([HAIKU_HOST], [1], [this is a Haiku platform])
    ;;
  *-gnu*)
    gnu=yes
    AC_DEFINE([GNU_HOST], [1], [this is a GNU platform])
    ;;
  *-*wince)
    wince=yes
    AC_DEFINE([WINCE_HOST], [1], [this is a WINCE platform])
    ;;
  *-*winmo)
    wince=yes
    AC_DEFINE([WINCE_HOST], [1], [this is a WINCE platform])
    AC_DEFINE([WINCE6_HOST], [1], [this is a WINCE6 platform])
    ;;
  *-android*)
    android=yes
    AC_DEFINE([ANDROID_HOST], [1], [this is an Android platform])
    ;;
esac

HOST="$host"
AC_SUBST([HOST])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h unistd.h sys/types.h sys/stat.h])
AC_CHECK_HEADERS([stdint.h stdio.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_INT16_T

# Checks for library functions.
AC_CHECK_FUNCS([open write close fprintf strcmp printf sscanf perror])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
