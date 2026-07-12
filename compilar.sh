#!/bin/bash

make

qemu-system-x86_64 -kernel build/os.bin -display curses
