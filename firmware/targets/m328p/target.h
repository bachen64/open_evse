// -*- C++ -*-
// Copyright (C) 2015 Sam C. Lin
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// See LICENSE for a copy of the GNU General Public License or see
// it online at <http://www.gnu.org/licenses/>.

#pragma once
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <pins_arduino.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Arduino.h"
#include "./Wire.h"
#include "./RTClib.h"


// enable $GI
#define MCU_ID_LEN 10

#define ADC_RESOLUTION_BITS 10
#define ADC_MAX 1023
#define ADC_HALF 512

// for J1772.ReadPilot()
// 1x = 114us 20x = 2.3ms 100x = 11.3ms
#define PILOT_LOOP_CNT 100



// This multiplier is the number of milliamps per A/d converter unit.

// First, you need to select the burden resistor for the CT. You choose the largest value possible such that
// the maximum peak-to-peak voltage for the current range is 5 volts. To obtain this value, divide the maximum
// outlet current by the Te. That value is the maximum CT current RMS. You must convert that to P-P, so multiply
// by 2*sqrt(2). Divide 5 by that value and select the next lower standard resistor value. For the reference
// design, Te is 1018 and the outlet maximum is 30. 5/((30/1018)*2*sqrt(2)) = 59.995, so a 56 ohm resistor
// is called for. Call this value Rb (burden resistor).

// Next, one must use Te and Rb to determine the volts-per-amp value. Note that the readCurrent()
// method calculates the RMS value before the scaling factor, so RMS need not be taken into account.
// (1 / Te) * Rb = Rb / Te = Volts per Amp. For the reference design, that's 55.009 mV.
// Each count of the A/d converter is 4.882 mV (5/1024). V/A divided by V/unit is unit/A. For the reference
// design, that's 11.26. But we want milliamps per unit, so divide that into 1000 to get 88.7625558. Round near...
//#define DEFAULT_CURRENT_SCALE_FACTOR 106 // for RB = 47 - recommended for 30A max
//#define DEFAULT_CURRENT_SCALE_FACTOR 184 // for RB = 27 - recommended for 50A max
// Craig K, I arrived at 213 by scaling my previous multiplier of 225 down by the ratio of my panel meter reading of 28 with the OpenEVSE uncalibrated reading of 29.6
// then upped the scale factor to 220 after fixing the zero offset by subtracing 900ma
//#define DEFAULT_CURRENT_SCALE_FACTOR 220 // for RB = 22 - measured by Craig on his new OpenEVSE V3
// NOTE: setting DEFAULT_CURRENT_SCALE_FACTOR TO 0 will disable the ammeter
// until it is overridden via RAPI
//#define DEFAULT_CURRENT_SCALE_FACTOR 220   // Craig K, average of three OpenEVSE controller calibrations
#ifdef OPENEVSE_2
#define DEFAULT_CURRENT_SCALE_FACTOR 186   // OpenEVSE II with a 27 Ohm burden resistor, after a 2-point calibration at 12.5A and 50A
#else
#define DEFAULT_CURRENT_SCALE_FACTOR 220   // OpenEVSE v2.5 and v3 with a 22 Ohm burden resistor (note that the schematic may say 28 Ohms by mistake)
#endif

// subtract this from ammeter current reading to correct zero offset
#ifdef OPENEVSE_2
#define DEFAULT_AMMETER_CURRENT_OFFSET 230 // OpenEVSE II with a 27 Ohm burden resistor, after a 2-point calibration at 12.5A and 50A
#else
#define DEFAULT_AMMETER_CURRENT_OFFSET 0   // OpenEVSE v2.5 and v3 with a 22 Ohm burden resistor.  Could use a more thorough calibration exercise to nails this down.
#endif



typedef unsigned long time_t;

#ifdef __cplusplus
class CriticalSection {
  uint8_t sreg;
public:
  CriticalSection() { start(); }
  void start() { sreg = SREG; cli(); }
  void end() { SREG = sreg; }
};

class AutoCriticalSection {
  uint8_t sreg;
public:
  AutoCriticalSection() { sreg = SREG; cli(); }
  ~AutoCriticalSection() { SREG = sreg; }
};

#endif // __cplusplus


// n.b. NONE OF THE PIN FUNCTIONS BELOW HANDLE PORTS > 0x100.. must be wrapped
// in critical section.. see Marlin fastio.h for details


//
// digital pin macros .. ugly, but no RAM usage
//
// don't use _xxx() versions
#define _DPIN_READ(reg,idx) (PIN ## reg & (1 << idx))
#define _DPIN_SET(reg,idx) {PORT ## reg |= (1<<idx);}
#define _DPIN_CLR(reg,idx) {PORT ## reg &= ~(1<<idx);}
#define _DPIN_WRITE(reg,idx,val) {if (val) DPIN_SET(reg,idx); else DPIN_CLR(reg,idx); }
#define _DPIN_MODE_INPUT(reg,idx) {DDR ## reg &= ~(1<<idx);}
#define _DPIN_MODE_PULLUP(reg,idx) { DPIN_SET(reg,idx);DPIN_MODE_INPUT(reg,idx);}
#define _DPIN_MODE_OUTPUT(reg,idx) {DDR ## reg |= (1<<idx);}

// use these
#define DPIN_READ(reg,idx) _DPIN_READ(reg,idx)
#define DPIN_SET(reg,idx) _DPIN_SET(reg,idx)
#define DPIN_CLR(reg,idx) _DPIN_CLR(reg,idx) 
#define DPIN_WRITE(reg,idx,val) _DPIN_WRITE(reg,idx,val) 
 // for (PINx > (uint8_t *)0x100 *must* use DPIN_WRITE_ATOMIC() instead of DPIN_WRITE()
#define DPIN_WRITE_ATOMIC(reg,idx,val) {AutoCriticalSection acs;_DPIN_WRITE(reg,idx,val)}
#define DPIN_MODE_INPUT(reg,idx) _DPIN_MODE_INPUT(reg,idx)
#define DPIN_MODE_PULLUP(reg,idx) _DPIN_MODE_PULLUP(reg,idx)
#define DPIN_MODE_OUTPUT(reg,idx) _DPIN_MODE_OUTPUT(reg,idx)
/*
example
#define LED_PORT B
#define LED_IDX  5

  DPIN_MODE_OUTPUT(B,LED_IDX);
  DPIN_SET(LED_PORT,LED_IDX);
  DPIN_CLR(LED_PORT,LED_IDX);
 */

//
// end digital pin macros
//


//
// pin macros .. ugly, but no RAM usage
//

#ifdef __cplusplus

void getMcuId(uint8_t *mcuid);

#if WATCHDOG_TIMEOUT_SEC != 2
#error "unsupported WATCHDOG_TIMEOUT_SEC value"
#endif

#ifdef WATCHDOG
#define WDT_RESET() wdt_reset()
#define WDT_ENABLE() wdt_enable(WDTO_2S)
#define WDT_ENABLE_1S() wdt_enable(WDTO_1S)
#define WDT_DISABLE() wdt_disable()
#else
#define WDT_RESET()
#define WDT_ENABLE()
#define WDT_ENABLE_1S()
#define WDT_DISABLE()
#endif // WATCHDOG

//
// begin digitalPin class
// using this class beautifies the code, but wastes 3 bytes per pin
//
class DigitalPin {
  volatile uint8_t* reg;
  uint8_t bit;
  
public:
  enum PinMode { INP,INP_PU,OUT };

  DigitalPin() {}
  DigitalPin(volatile uint8_t* _reg,uint8_t idx,PinMode _mode) {
    init(_reg,idx,_mode);
  }

  void init(volatile uint8_t* _reg,uint8_t idx,PinMode _mode);

  void mode(PinMode mode);
  uint8_t read() {
#ifdef HIGH
    return (*pin() & bit) ? HIGH : LOW;
#else
    return *pin() & bit;
#endif
  }
  void write(uint8_t state) {
    if (state) *port() |= bit;
    else *port() &= ~bit;
  }
  // for (PINx > (uint8_t *)0x100 *must* use writeAtomic() instead of write()
  void writeAtomic(uint8_t state) {
    AutoCriticalSection acs;
    write(state);
  }  

  volatile uint8_t* pin() { return reg; }
  volatile uint8_t* ddr() { return reg+1; }
  volatile uint8_t* port() { return reg+2; }

};


//
// begin AdcPin class
// using this class beautifies the code, but wastes 1 byte per pin
//
class AdcPin {
  static uint8_t refMode;
  uint8_t channel;
  
public:
  enum PinMode { INP,INP_PU,OUT };

  AdcPin() {}
  AdcPin(uint8_t _adcNum) {
    init(_adcNum);
  }

  void init(uint8_t _adcNum);
  uint16_t read();

  static void referenceMode(uint8_t mode) {
    refMode = mode;
  }
};

//  why double up on these macros? see http://gcc.gnu.org/onlinedocs/cpp/Stringification.html
// don't call the _xxx() version directly
#define _DIGITAL_PIN(name,reg,idx,mode) name(&PIN ## reg,idx,mode)
#define DIGITAL_PIN(name,reg,idx,mode) _DIGITAL_PIN(name,reg,idx,mode)

/*
examples:
// pin B6: reg=B,pin=6
DigitalPin DIGITAL_PIN(mypin,B,6,DigitalPin::INP);
mypin.write(HIGH);
uint8_t val = mypin.read();

DigitalPin mypin2(&PINB,6,DigitalPin::INP);

class Led {
  DigitalPin pin;
public:
  Led() : DIGITAL_PIN(pin,B,5,DigitalPin::OUT) {}
  void set(uint8_t onoff) { write(onoff); {
};
*/

//
// end digital pin class
//

void initTarget();
#endif // __cplusplus
