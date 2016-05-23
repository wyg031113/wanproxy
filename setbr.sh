#!/bin/bash
brctl addbr virbr0
brctl addif virbr0 em1
brctl addif virbr0 p4p1
ifconfig virbr0 192.168.1.200
brctl show virbr0
