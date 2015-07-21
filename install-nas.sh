#!/bin/bash
# Installs compiled stuff into Synology NAS

if [ ! -e rawfs-arm ]; then
    echo "rawfs-arm doesn't exist, run build-arm.sh first"
    exit 1
fi

NAS=nas.local
PHOTOS=/volume1/Photos
MOUNTPOINT=/volume1/photo
PREFIX=/opt
START_SCRIPT=$PREFIX/etc/init.d/S99rawfs

scp libfuse.so.2 rawfs-arm $NAS:$PREFIX/bin
ssh $NAS "echo 'LD_LIBRARY_PATH=$PREFIX/bin $PREFIX/bin/rawfs-arm $PHOTOS $MOUNTPOINT -o allow_other' > $START_SCRIPT;
chmod a+x $START_SCRIPT;
$START_SCRIPT"
