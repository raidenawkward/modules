#!/bin/bash

module="scull"
device="scull"
mode="664"

INSMOD=/sbin/insmod

$INSMOD $module.ko $* || exit 1

rm -f /dev/${device}[0-3]

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)
type="c"

for i in {0 1 2 3}
do
	mknod /dev/${device}$i $type $major $i
done

group="staff"
grep -q '^staff:' /etc/group || group="wheel"

chgrp $group /dev/${device}[0-3]
chmod $mode /dev/${device}[0-3]
