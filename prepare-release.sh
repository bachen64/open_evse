#!/bin/bash

# Script to compile all released environments and copy build artifacts to firmware/targets/ for easy upload to github releases.

mkdir -p firmware/targets

pio run -e m328p_core
cp .pio/build/m328p_core/firmware.hex firmware/targets/openevse_core.hex

pio run -e m328p_noWiFi
cp .pio/build/m328p_noWiFi/firmware.hex firmware/targets/openevse_noWiFi.hex

pio run -e m328p_LCD_WIFI
cp .pio/build/m328p_LCD_WIFI/firmware.hex firmware/targets/openevse_legacy.hex

pio run -e samd
cp .pio/build/samd/firmware.bin firmware/targets/arm_samd_openevse_nxt.bin

ls firmware/targets/

echo "Done"
