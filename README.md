rawfs
=====

FUSE filesystem that shows RAW image files (eg CR2) as their embedded JPEGs.

Very useful if you have bunch of apps that you want to use on your photos
that don't support RAW files properly.

Build
-----

On Ubuntu:
 sudo apt-get install libfuse-dev

Then:
 ./buid.sh

Usage
-----

 ./rawfs mountpoint
