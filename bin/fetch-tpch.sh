#!/bin/bash

DATA_FILE=http://www-db.in.tum.de/teaching/ss13/qo/tpch.7z
OUTPUT_FILE=tpch.7z
TARGET_DIR=data

cd `dirname $0`/..

command -v wget >/dev/null 2>&1 || {
  command -v curl >/dev/null 2>&1 || {
    echo >&2 "Neither wget nor curl available. Cannot run."
    exit 1
  }
  dl="curl -C - $DATA_FILE -o $TARGET_DIR/$OUTPUT_FILE"
}

if [ -z "$dl" ]; then
  dl="wget --continue $DATA_FILE -O $TARGET_DIR/$OUTPUT_FILE"
fi

mkdir -p $TARGET_DIR

$dl
7z x -o$TARGET_DIR -y $TARGET_DIR/$OUTPUT_FILE
sed --in-place "s/;/,/g" $TARGET_DIR/tpch/*.tbl
chmod a+x $TARGET_DIR/tpch/loadtpch-cpp
