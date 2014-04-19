#!/bin/sh

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 tap_name"
    exit 1
fi

sudo ip tuntap del mode tap dev $1
