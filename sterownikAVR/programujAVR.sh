#!/bin/bash
avr-gcc -g -Os -mmcu=atmega88pa -c programAVR.c
avr-gcc -g -mmcu=atmega88pa -o programAVR.elf programAVR.o
avr-objcopy -j .text -j .data -O ihex programAVR.elf programAVR.hex
sudo /usr/bin/avrdude -p m88p -c linuxgpio -U flash:w:programAVR.hex
sudo rmmod spi_bcm2835
sudo modprobe spi_bcm2835
