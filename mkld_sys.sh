#!/bin/bash
cd /home/scanel/botker
./make.sh
qemu -fda bootsect.bin -fdb kernel.bin -curses
