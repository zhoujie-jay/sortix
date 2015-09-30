set -e

make_dir_path_absolute() {
  (cd "$1" && pwd)
}

has_command() {
  which "$1" > /dev/null
}

# Detect if the environment isn't set up properly.
if [ -z "$SORTIX_PORTS_DIR" ]; then
  echo "$0: error: You need to set \$SORTIX_PORTS_DIR" >&2
  exit 1
elif ! [ -d "$SORTIX_PORTS_DIR" ] ||
     [ "$(ls "$SORTIX_PORTS_DIR") | wc -l" = 0 ]; then
  exit 0
elif ! has_command tix-build; then
  echo "$0: warning: Can't clean ports directory without Tix locally installed." >&2
  exit 0
fi

# Make paths absolute for later use.
SORTIX_PORTS_DIR=$(make_dir_path_absolute "$SORTIX_PORTS_DIR")

# Detect all packages.
get_all_packages() {
  for PACKAGE in $(ls "$SORTIX_PORTS_DIR"); do
    ! [ -f "$SORTIX_PORTS_DIR/$PACKAGE/tixbuildinfo" ] ||
    echo $PACKAGE
  done
}

# Clean all the packages.
for PACKAGE in $(get_all_packages); do
  [ -f "$SORTIX_REPOSITORY_DIR/$PACKAGE.tix.tar.xz" ] ||
  tix-build \
    --sysroot="/" \
    --host=$HOST \
    --prefix= \
    --destination="/" \
    --start=clean \
    --end=clean \
    "$SORTIX_PORTS_DIR/$PACKAGE"
done
