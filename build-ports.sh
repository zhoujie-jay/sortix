#!/bin/sh

set -e

make_dir_path_absolute() {
  (cd "$1" && pwd)
}

has_command() {
  which "$1" > /dev/null
}

# Detect if the environment isn't set up properly.
if [ -z "$PACKAGES" -a "${PACKAGES+x}" = 'x' ]; then
  exit 0
elif [ -z "$HOST" ]; then
  echo "$0: error: You need to set \$HOST" >&2
  exit 1
elif [ -z "$SYSROOT" ]; then
  echo "$0: error: You need to set \$SYSROOT" >&2
  exit 1
elif [ -z "$SORTIX_PORTS_DIR" ]; then
  echo "$0: error: You need to set \$SORTIX_PORTS_DIR" >&2
  exit 1
elif [ -z "$SORTIX_REPOSITORY_DIR" ]; then
  echo "$0: error: You need to set \$SORTIX_REPOSITORY_DIR" >&2
  exit 1
elif ! [ -d "$SORTIX_PORTS_DIR" ]; then
  echo "Warning: No ports directory found, third party software will not be built"
  exit 0
elif ! has_command tix-collection ||
     ! has_command tix-build ||
     ! has_command tix-install; then
  echo "$0: error: You need to have installed Tix locally to compile ports." >&2
  exit 1
fi

# Add the platform triplet to the binary repository path.
SORTIX_REPOSITORY_DIR="$SORTIX_REPOSITORY_DIR/$HOST"
mkdir -p "$SORTIX_REPOSITORY_DIR"

# Make paths absolute for later use.
SYSROOT=$(make_dir_path_absolute "$SYSROOT")
SORTIX_PORTS_DIR=$(make_dir_path_absolute "$SORTIX_PORTS_DIR")
SORTIX_REPOSITORY_DIR=$(make_dir_path_absolute "$SORTIX_REPOSITORY_DIR")

# Create a temporary directory in which out-of-directory builds will happen.
if [ -z "$BUILDTMP" ]; then
  export BUILDTMP=$(mktemp -d)
fi

# Decide the optimization options with which the ports will be built.
if [ -z "${OPTLEVEL+x}" ]; then OPTLEVEL="-Os"; fi
if [ -z "${PORTS_OPTLEVEL+x}" ]; then PORTS_OPTLEVEL="$OPTLEVEL"; fi
if [ -z "${PORTS_CFLAGS+x}" ]; then PORTS_CFLAGS="$PORTS_OPTLEVEL"; fi
if [ -z "${PORTS_CXXFLAGS+x}" ]; then PORTS_CXXFLAGS="$PORTS_OPTLEVEL"; fi
if [ -z "${CFLAGS+x}" ]; then CFLAGS="$PORTS_CFLAGS"; fi
if [ -z "${CXXFLAGS+x}" ]; then CXXFLAGS="$PORTS_CXXFLAGS"; fi
export CFLAGS
export CXXFLAGS

# Detect all packages.
get_all_packages() {
  for PACKAGE in $(ls "$SORTIX_PORTS_DIR"); do
    ! [ -f "$SORTIX_PORTS_DIR/$PACKAGE/tixbuildinfo" ] ||
    echo $PACKAGE
  done
}

# Detect which packages are available if not specified.
if [ -z "${PACKAGES+x}" ]; then
  PACKAGES=$(get_all_packages | sort -R)
fi

# Simply stop if there is no packages available.
if [ -z "$PACKAGES" ]; then
  exit 0
fi

# Detect the build-time dependencies for a package.
get_package_dependencies_raw() {(
  PACKAGE_DIR=$(echo $1 | grep -Eo '^[^\.]*')
  ! [ -f "$SORTIX_PORTS_DIR/$PACKAGE_DIR/tixbuildinfo" ] ||
  grep -E "^pkg\.build-libraries=.*" "$SORTIX_PORTS_DIR/$PACKAGE_DIR/tixbuildinfo" | \
  sed 's/^pkg\.build-libraries=//'
)}

# Detect the build-time dependencies for a package with missing optional
# dependencies removed.
get_package_dependencies() {(
  PRINTED_ANY=false
  for DEPENDENCY in $(get_package_dependencies_raw $1); do
    if [ "$DEPENDENCY" != "${DEPENDENCY%\?}" ]; then
      DEPENDENCY="${DEPENDENCY%\?}"
      FOUND=false
      for PACKAGE in $PACKAGES; do
        if [ "$PACKAGE" = "$DEPENDENCY" ]; then
           FOUND=true
           break
        fi
      done
      if ! $FOUND; then
        continue
      fi
    fi
    if $PRINTED_ANY; then echo -n ' '; fi
    echo -n "$DEPENDENCY"
    PRINTED_ANY=true
  done
  if $PRINTED_ANY; then echo; fi
)}

# Decide the order the packages are built in according to their dependencies.
DEPENDENCY_MAKEFILE=$(mktemp)
(for PACKAGE in $PACKAGES; do
   echo "$PACKAGE: $(get_package_dependencies $PACKAGE)"
   echo "	@echo $PACKAGE"
 done;
 echo -n ".PHONY:"
 for PACKAGE in $PACKAGES; do
   echo -n " $PACKAGE"
 done;
 echo) > "$DEPENDENCY_MAKEFILE"

BUILD_LIST=$(unset MAKE;
             unset MFLAGS;
             unset MAKEFLAGS;
             make -Bs -f "$DEPENDENCY_MAKEFILE" $PACKAGES)
rm -f "$DEPENDENCY_MAKEFILE"
PACKAGES="$BUILD_LIST"

# Create the system root if absent.
mkdir -p "$SYSROOT"

# Create the binary package repository.
mkdir -p "$SORTIX_REPOSITORY_DIR"

# Initialize Tix package management in the system root if absent.
[ -d "$SYSROOT/tix/$HOST" ] ||
tix-collection "$SYSROOT" create --platform=$HOST --prefix=

# Build all the packages (if needed) and otherwise install them.
for PACKAGE in $PACKAGES; do
  [ -f "$SORTIX_REPOSITORY_DIR/$PACKAGE.tix.tar.xz" ] ||
  tix-build \
    --sysroot="$SYSROOT" \
    --host=$HOST \
    --prefix= \
    --destination="$SORTIX_REPOSITORY_DIR" \
    "$SORTIX_PORTS_DIR/$PACKAGE"
  tix-install \
    --collection="$SYSROOT" \
    --reinstall \
    "$SORTIX_REPOSITORY_DIR/$PACKAGE.tix.tar.xz"
done
