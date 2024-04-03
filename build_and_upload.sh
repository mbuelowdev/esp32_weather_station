#!/bin/bash

idf.py build

sshpass -p "$1" scp build/bootloader/bootloader.bin build/partition_table/partition-table.bin build/weather_station.bin CMakeLists.txt michael@usbpi.taco.open0x20.de:/builder/

exit 0


# Afterwards execute the following on the usbpi host:
python -m esptool --chip esp32 -b 460800 --before default_reset \
--after hard_reset write_flash --flash_mode dio --flash_size detect \
--flash_freq 40m 0x1000 /builder/bootloader.bin 0x8000 /builder/partition-table.bin 0x10000 /builder/weather_station.bin \
&& idf.py monitor