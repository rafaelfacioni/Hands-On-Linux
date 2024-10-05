#!/bin/bash

make
rmmod serial_write
rmmod cp210x
insmod serial_write.ko