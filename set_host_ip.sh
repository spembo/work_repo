#!/bin/bash

IFACE=$1

sudo ip addr flush dev ${IFACE}
sudo ip addr add 192.168.1.81/24 dev ${IFACE}
