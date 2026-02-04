rem avrdude -c USBasp -p m328p -B16 -U lfuse:w:0xFF:m -U hfuse:w:0xDF:m -U efuse:w:0xFD:m
avrdude -c USBasp -p m328p -B0.5 -U flash:w:%1
