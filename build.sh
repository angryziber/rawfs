gcc rawfs.c -Wall -std=gnu99 `pkg-config fuse --cflags --libs`  -o rawfs
