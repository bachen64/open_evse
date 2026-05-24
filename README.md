# OpenEVSE

Firmware for OpenEVSE controller used in OpenEVSE Charging Stations.

- OpenEVSE: <https://store.openevse.com/collections/all-products>


Based on OpenEVSE: Open Source Hardware J1772 Electric Vehicle Supply Equipment


## API Documentation

- WIFI API: <http://github.com/openevse/ESP32_WiFi_V4.x/>


## Resources

- [OpenEVSE Controller Hardware Repo](https://github.com/OpenEVSE/OpenEVSE_PLUS)
- [OpenEVSE Project Homepage](https://openevse.com)

***

Firmware compile & upload help: [firmware/open_evse/LoadingFirmware.md](firmware/open_evse/LoadingFirmware.md)

NOTES:

- Working versions of the required libraries are included with the firmware code. This avoids potential issues related to using the wrong versions of the libraries.
- Highly recommend using the tested pre-compiled firmware (see releases page)

## Flash pre-compiled using avrdude

`$ avrdude -p atmega328p -B6 -c usbasp -P usb -e -U flash:w:firmware.hex`

ISP programmer required

### Set AVR fuses

This only needs to be done once in the factory

`avrdude -c USBasp -p m328p -U lfuse:w:0xFF:m -U hfuse:w:0xDF:m -U efuse:w:0xFD:m -B6`

If writing eFuse fails ISBasp may need a [firmware update](https://www.vishnumaiea.in/articles/electronics/how-to-solve-usbasp-avr-efuse-write-problem-on-progisp)

***

Tip Jar: Lincomatic developed/maintain this firmware on a volunteer basis. Any donation, no matter how small, is greatly appreciated.

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.me/lincomatic)

```text
Open EVSE is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

Open EVSE is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Open EVSE; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

* Open EVSE is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
