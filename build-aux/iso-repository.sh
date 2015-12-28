#!/bin/sh -e

# Detect if the environment isn't set up properly.
if [ -z "$HOST" ]; then
  echo "$0: error: You need to set \$HOST" >&2
  exit 1
elif [ -z "$SORTIX_REPOSITORY_DIR" ]; then
  echo "$0: error: You need to set \$SORTIX_REPOSITORY_DIR" >&2
  exit 1
fi

if ! [ -d "$SORTIX_REPOSITORY_DIR" ]; then
  exit 0
fi
SORTIX_REPOSITORY_DIR="$SORTIX_REPOSITORY_DIR/$HOST"
if ! [ -d "$SORTIX_REPOSITORY_DIR" ]; then
  exit 0
fi

mkdir -p "$1"

if [ -z "${PACKAGES+x}" ]; then
  cp -RT "$SORTIX_REPOSITORY_DIR" "$1"
else
  for PACKAGE in $PACKAGES; do
    cp "$SORTIX_REPOSITORY_DIR/$PACKAGE.tix.tar.xz" "$1"
  done
fi
