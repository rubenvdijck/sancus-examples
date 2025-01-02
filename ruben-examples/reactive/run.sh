#! /bin/bash
# Build project
set -e

make all

VENDOR_KEY=0b7bf3ae40880a8be430d0da34fb76f0 # 4078d505d82099ba

# Fill in macs of the Sancus Modules if they exist
sancus-crypto --key $VENDOR_KEY --fill-macs bin/sancus-msp430/reactive.elf -o macs.elf

# Destroy potential old screen session
screen -X -S riot_prints quit || true
# Load elf file to device
sancus-loader -device /dev/ttyUSB0 macs.elf
# Create new named screen session to listen for prints
screen -S riot_prints /dev/ttyUSB0 57600
