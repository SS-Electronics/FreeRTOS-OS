#!/bin/bash


############### Configuration #########################
GDB=arm-none-eabi-gdb




#######################################################




@echo "Debugging..."
echo $OPENOCD_PATH
$GDB -ex 'target remote localhost:3333'  ./build/freertos_kernel.elf
