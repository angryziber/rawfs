#!/bin/bash
# Installs compiled stuff into Synology NAS

if [ ! -e rawfs-arm ]; then
    echo "rawfs-arm doesn't exist, run build-arm.sh first"
    exit 1
fi

NAS=nas.local
PHOTOS=/volume1/Photos
MOUNTPOINT=/volume1/photo

scp libfuse.so.2 rawfs-arm $NAS:
ssh $NAS "LD_LIBRARY_PATH=. ./rawfs-arm $PHOTOS $MOUNTPOINT -o allow_other"
