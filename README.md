rawfs
=====

FUSE filesystem that shows RAW image files (eg CR2) as their embedded JPEGs.

Very useful if you have bunch of apps that you want to use on your photos
that don't support RAW files properly.

Build
-----

On Ubuntu:
```bash
sudo apt-get install libfuse-dev
```

Then:
```bash
 ./buid.sh
```

Usage
-----

```bash
./rawfs mountpoint
```

Synology NAS
------------

It makes sense to cross-compile rawfs for your Synology NAS.

Steps:

1. Download Synology toolchain from http://sf.net/projects/dsgpl/files/
   Get the right one for your DSM software version and CPU (my DS212j is Marvell 88F628x)
2. Download FUSE sources from http://sf.net/projects/fuse/ (the NAS already has the kernel module)
3. Extract both side-to-side into the same directory
4. To cross-compile FUSE:

```bash
  CPPFLAGS=-I`pwd`/../arm-none-linux-gnueabi/include/ CC=`pwd`/../arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-gcc ./configure --host=arm-none-linux-gnueabi --prefix=`pwd`/target
  make
  make install
```

5. Clone rawfs side-by-side with fuse and the toolchain
6. ./build-arm.sh - this will get you a nice rawfs-arm binary! Unfortunately, I didn't manage to link it with -static, so you will also need libfuse.so.2
7. Now upload both rawfs-arm and libfuse.so.2 to your device and run with

```
LD_LIBRARY_PATH=. ./rawfs-arm
```

