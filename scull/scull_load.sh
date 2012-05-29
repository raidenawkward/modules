#!/bin/bash

module="scull"
device="scull"
mode="664"

INSMOD=/sbin/insmod

$INSMOD $module.ko $* || exit 1

rm -f /dev/${device}[0-3]

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)
if [ -z $major ]
then
	major=1234
fi

group="staff"
grep -q '^staff:' /etc/group || group="wheel"

type="c"

nodes="0 1 2 3"
for i in $nodes
do
	mknod /dev/$device$i $type $major $i
	chgrp $group /dev/${device}$i
	chmod $mode /dev/${device}$i
done
