#pragma once


#define ADC_RESOLUTION_BITS 12
#define ADC_MAX 4095
#define ADC_HALF 2048

// for J1772.ReadPilot()
// 15 = ~13ms
#define PILOT_LOOP_CNT 15


#define GetVerStr(s) strcpy(s,VERSION)

#include "eeprom_emulation.h"

void wdt_disable();

#ifdef WATCHDOG
#include <Adafruit_SleepyDog.h>

inline void wdt_enable(int sec)
{
  Watchdog.enable(sec * 1000);
}

inline void wdt_disable()
{
  Watchdog.disable();
}

inline void wdt_reset()
{
  Watchdog.reset();
}

#define WDT_RESET() wdt_reset()
#define WDT_ENABLE() wdt_enable(2)
#define WDT_ENABLE_1S() wdt_enable(1)
#define WDT_DISABLE() wdt_disable()
#else
#define WDT_RESET()
#define WDT_ENABLE()
#define WDT_ENABLE_1S()
#define WDT_DISABLE()
#define wdt_reset()
#define wdt_enable(sec)
#endif // WATCHDOG

class DigitalPin {
  uint32_t _pinNum;
  uint32_t _pinMode;
  
public:
  enum PinMode { INP,INP_PU,OUT };

  DigitalPin() {}
  DigitalPin(uint32_t pinnum,int idxjunk,PinMode mode) {
    init(pinnum,idxjunk,mode);
  }

void init(uint32_t pinnum,int idxjunk,PinMode mode);
  void mode(PinMode mode);

  uint8_t read() {
    return digitalRead(_pinNum) ? 1 : 0;
  }
  void write(uint32_t state) {
    digitalWrite(_pinNum,state ? HIGH : LOW);
  }
};

class AdcPin {
  uint32_t _pinNum;
public:

  AdcPin() {}
  AdcPin(uint32_t pinnum) {
    init(pinnum);
  }

  void init(uint32_t pinnum) {
    _pinNum = pinnum;
    analogReadResolution(ADC_RESOLUTION_BITS);
  }

  uint32_t read() {
    return analogRead(_pinNum);
  }
};

void getMcuId(uint8_t *mcuid);

void initTarget();
