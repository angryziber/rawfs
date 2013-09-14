rawfs
=====

FUSE filesystem that shows RAW image files (eg Canon CR2) as their embedded JPEGs.

Very useful if you have bunch of apps that you want to use on your photos
that don't support RAW files properly.

You can easily mount your RAW photos directory to another location
and browse it with a JPEG-only tool without noticing. 

Unsupported files will be preserved as they are. No disk space is used when mounting.
JPEG thumbnails are served directly from the original RAW files.

The original exif data is also added to the jpegs, so that autorotation, etc will work.

Limitations
-----------

* Filesystem is read-only, thus only for browsing

Download
--------

You can download Linux 64-bit binaries and Synology ARM binaries from [github releases](https://github.com/angryziber/rawfs/releases).

For Synology you will need both libfuse.so.2 and rawfs-arm.

Usage
-----

Mounting
```bash
./rawfs photosdir mountpoint
```

Unmounting
```bash
fusermount -u mountpoint
```
or
```bash
sudo umount mountpoint
```

Just extract the embedded jpeg
```bash
./rawextract filename
```

Mounting on Synology NAS
```bash
LD_LIBRARY_PATH=. ./rawfs-arm photosdir mountpoint -o allow_other
```
allow_other option will allow other users to see the mounted dir (by default in Fuse, only the user who ran the mount can see the files)

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

Synology NAS
------------

Steps to cross-compile:

1. Download Synology toolchain from http://sf.net/projects/dsgpl/files/
   Get the right one for your DSM software version and CPU (my DS212j is Marvell 88F628x)
2. Download FUSE sources from http://sf.net/projects/fuse/ (the NAS already has the kernel module)
3. Extract both side-to-side into the same directory
4. To cross-compile FUSE (from the fuse dir):
   ```bash
      export CPPFLAGS=-I`pwd`/../arm-none-linux-gnueabi/include/
      export CC=`pwd`/../arm-none-linux-gnueabi/bin/arm-none-linux-gnueabi-gcc
      ./configure --host=arm-none-linux-gnueabi --prefix=`pwd`/target
      make
      make install
   ```

5. Clone rawfs side-by-side with fuse and the toolchain
6. ./build-arm.sh - this will get you a nice rawfs-arm binary! Unfortunately, I didn't manage to link it with -static, so you will also need libfuse.so.2
7. Now upload both rawfs-arm and libfuse.so.2 to your device and run as indicated in the Usage section above
