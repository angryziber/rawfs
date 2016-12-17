#!/bin/bash
# Installs compiled stuff into Synology NAS

if [ ! -e rawfs-arm ]; then
    echo "rawfs-arm doesn't exist, run build-arm.sh first"
    exit 1
fi

NAS=nas.local
PHOTOS=/volume1/Photos
MOUNTPOINT=/volume1/photo
DEST=/root
START_SCRIPT=/etc/init.d/S99rawfs

scp rawfs-arm $NAS:$DEST
ssh $NAS "echo '$DEST/rawfs-arm $PHOTOS $MOUNTPOINT -o allow_other' > $START_SCRIPT;
chmod a+x $START_SCRIPT;
rm -fr $MOUNTPOINT/@eaDir;
$START_SCRIPT"
