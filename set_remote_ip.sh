#!/bin/bash

IFACE=$1

sudo ip addr flush dev ${IFACE}
sudo ip addr add 192.168.1.80/24 dev ${IFACE}
