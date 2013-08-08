#!/bin/bash

HOST=arm-none-linux-gnueabi
FUSE=../fuse-2.9.3

../$HOST/bin/$HOST-gcc -D_FILE_OFFSET_BITS=64 -I$FUSE/target/include/ -lfuse -std=gnu99 -L$FUSE/target/lib/ -o rawfs-arm rawfs.c
