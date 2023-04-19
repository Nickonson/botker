#!/bin/bash
cd /home/scanel/botker
as --32 -o bootsect.o bootsect.asm
ld -Ttext 0x7c00 --oformat binary -m elf_i386 -o bootsect.bin bootsect.o
g++ -fno-pie -ffreestanding -m32 -o kernel.o -c kernel.cpp
ld --oformat binary -Ttext 0x10000 -o kernel.bin --entry=kmain -m elf_i386 kernel.o
