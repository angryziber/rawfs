#!/bin/bash

HOST=arm-marvell-linux-gnueabi
FUSE=../fuse-2.9.3

../$HOST/bin/$HOST-gcc -D_FILE_OFFSET_BITS=64 -I$FUSE/target/include/ -lfuse -std=gnu99 -L$FUSE/target/lib/ -o rawfs-arm rawfs.c || exit
cp -L $FUSE/target/lib/libfuse.so.2 . || exit
echo "Now upload rawfs-arm and libfuse.so.2 to your device and run with LD_LIBRARY_PATH=. ./rawfs-arm"
