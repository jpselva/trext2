#!/bin/bash

DISKIMG="disk.img"
MNTPOINT="/mnt/diskimg"

if [ $1 = "mount" ]; then # if command is mount...

    # create the mountpoint if it doesn't exist
    [ ! -d $MNTPOINT ] && mkdir $MNTPOINT

    # associate loop device 0 to disk.img
    losetup /dev/loop0 disk.img

    # mount loop device 0
    mount /dev/loop0 $MNTPOINT

elif [ $1 = "umount" ]; then # if command is umount...

    # umount the loop device
    umount $MNTPOINT

    # detach disk.img from loop device 0
    losetup -d /dev/loop0

    # remove mountpoint if it exists
    rmdir $MNTPOINT

else
    echo "unkown command"
fi
