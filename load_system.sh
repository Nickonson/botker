#!/bin/bash
cd /home/scanel/botker
qemu -fda bootsect.bin -fdb kernel.bin -curses
