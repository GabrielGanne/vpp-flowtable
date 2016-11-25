#!/bin/bash

vppctl set int state TenGigabitEthernet83/0/0 up
vppctl set int state TenGigabitEthernet83/0/1 up
vppctl set interface l2 bridge TenGigabitEthernet83/0/0 23
vppctl set interface l2 bridge TenGigabitEthernet83/0/1 23

vppctl create netmap name vale00:pm hw-addr 02:FE:3F:34:15:9B pipe master
vppctl set int state netmap-vale00:pm up
