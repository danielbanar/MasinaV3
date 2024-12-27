#!/bin/bash
ip link set wwan0 down
echo Y > /sys/class/net/wwan0/qmi/raw_ip
ip link set wwan0 up
qmicli -p -d /dev/cdc-wdm0 --wds-set-autoconnect-settings=enabled
qmicli -p -d /dev/cdc-wdm0 --device-open-net='net-raw-ip|net-no-qos-header' --wds-start-network="apn='o2internet',ip-type=4" --client-no-release-cid
udhcpc -q -i wwan0
