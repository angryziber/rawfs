GCC_OPTS="-Wall -std=gnu99 `pkg-config fuse --cflags --libs`"
gcc rawextract.c $GCC_OPTS -o rawextract &&
gcc rawfs.c $GCC_OPTS  -o rawfs
