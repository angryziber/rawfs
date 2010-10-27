gcc -Wall -std=gnu99 `pkg-config fuse --cflags --libs` rawfs.c -o rawfs
