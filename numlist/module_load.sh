#!/bin/bash

module="numlist"
device="numlist"
mode="777"

INSMOD=/sbin/insmod
RMMOD=/sbin/rmmod

WHO=`whoami`
if [ "$WHO" != "root" ]
then
	echo script needs root permission
	exit 1
fi

installed=`lsmod | grep $module`
if [ ! -z "$installed" ]
then
	$RMMOD $module
fi

$INSMOD $module.ko $* || exit 1

rm -f /dev/${device}

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)
if [ -z $major ]
then
	major=1234
	echo cannot find device major, use default instead
	#exit 1
fi

group="staff"
grep -q '^staff:' /etc/group || group="wheel"

type="c"

mknod /dev/${device} $type $major 1
chgrp $group /dev/${device}
chmod $mode /dev/${device}
