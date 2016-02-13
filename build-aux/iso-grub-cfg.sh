#!/bin/sh -e

platform=
directory=
version=

dashdash=
previous_option=
for argument do
  if test -n "$previous_option"; then
    eval $previous_option=\$argument
    previous_option=
    continue
  fi

  case $argument in
  *=?*) parameter=$(expr "X$argument" : '[^=]*=\(.*\)') ;;
  *=)   parameter= ;;
  *)    parameter=yes ;;
  esac

  case $dashdash$argument in
  --) dashdash=yes ;;
  --platform=*) platform=$parameter ;;
  --platform) previous_option=platform ;;
  --version=*) version=$parameter ;;
  --version) previous_option=version ;;
  -*) echo "$0: unrecognized option $argument" >&2
      exit 1 ;;
  *) directory="$argument" ;;
  esac
done

if test -n "$previous_option"; then
  echo "$0: option '$argument' requires an argument" >&2
  exit 1
fi

if test -z "$platform"; then
  echo "$0: platform wasn't set with --platform" >&2
  exit 1
fi

if test -z "$version"; then
  echo "$0: platform wasn't set with --version" >&2
  exit 1
fi

if test -z "$directory"; then
  echo "$0: no directory operand supplied" >&2
  exit 1
fi

machine=$(expr x"$platform" : 'x\([^-]*\).*')

cd "$directory"

mkdir -p boot/grub
exec > boot/grub/grub.cfg

cat << EOF
insmod part_msdos
insmod ext2
EOF
find . | grep -Eq '\.gz$' && echo "insmod gzio"
find . | grep -Eq '\.xz$' && echo "insmod xzio"

echo
cat << EOF
if loadfont unicode ; then
  insmod vbe
  insmod vga
  insmod gfxterm
fi
terminal_output gfxterm

set menu_title="Sortix $version for $machine"
set timeout=10
set default="0"
EOF

maybe_compressed() {
  if [ -e "$1.xz" ]; then
    echo "$1.xz"
  elif [ -e "$1.gz" ]; then
    echo "$1.gz"
  elif [ -e "$1" ]; then
    echo "$1"
  fi
}

human_size() {
  LC_ALL=C du -bh "$1" | grep -Eo '^[^[:space:]]+'
}

menuentry() {
  echo
  args=""
  [ -n "$2" ] && args=" $2"
  kernel=$(maybe_compressed boot/sortix.bin)
  live_initrd=$(maybe_compressed boot/live.initrd)
  overlay_initrd=$(maybe_compressed boot/overlay.initrd)
  src_initrd=$(maybe_compressed boot/src.initrd)
  system_initrd=$(maybe_compressed boot/system.initrd)
  printf "menuentry \"Sortix (%s)\" {\n" "$1"
  case $platform in
  x86_64-*)
    cat << EOF
	if ! cpuid -l; then
		echo "Error: You cannot run this 64-bit operating system because" \
		     "this computer has no 64-bit mode."
		read
		exit
	fi
EOF
    ;;
  esac
  cat << EOF
	echo -n "Loading /$kernel ($(human_size $kernel)) ... "
	multiboot /$kernel$args
	echo done
EOF
  for initrd in $system_initrd $src_initrd $live_initrd $overlay_initrd; do
  cat << EOF
	echo -n "Loading /$initrd ($(human_size $initrd)) ... "
	module /$initrd
	echo done
EOF
  done
  find repository | grep -E '^(.*/)?.*\.tix\.tar\.xz$' | LC_ALL=C sort | while read tix; do
    cat << EOF
	echo -n "Loading /$tix$I ($(human_size $tix)) ... "
	module /$tix
	echo done
EOF
  done
  printf "}\n"
}

menuentry "live environment" ''
