#!/bin/sh

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 tap_name ip/mask"
    exit 1
fi

sudo ip tuntap add mode tap dev $1
sudo ip addr add $2 dev $1
sudo ip addr show $1
