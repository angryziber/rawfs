#!/bin/bash
# To compile with GLIBC < 2.33 (for Synology, which uses GLIBC 2.20), run in:
# docker run -ti -v `pwd`:/app ubuntu:20.04 'apt update && apt install gcc pkg-config libfuse-dev && bash'
GCC_OPTS="-g -rdynamic -Wall -std=gnu99 -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs`"
gcc rawextract.c $GCC_OPTS -o rawextract &&
gcc rawfs.c $GCC_OPTS  -o rawfs
