#!/bin/bash

# Script to compile all released environments and copy build artifacts to firmware/targets/ for easy upload to github releases.

mkdir -p firmware/targets

pio run -e m328p_autodetect
cp .pio/build/m328p_v6/firmware.hex firmware/targets/m328p_openevse.hex

pio run -e m328p_RTC_BTN_LCD
cp .pio/build/m328p_RTC_BTN_LCD/firmware.hex firmware/targets/m328p_openevse_RTC_BTN_LCD.hex

pio run -e samd
cp .pio/build/samd_us/firmware.bin firmware/targets/arm_samd_openevse_nxt.bin

ls firmware/targets/

echo "Done"
