#if defined(ENABLE_DEBUG) && !defined(ENABLE_DEBUG_GFI)
#undef ENABLE_DEBUG
#endif

/*
 * This file is part of Open EVSE.
 *
 * Copyright (c) 2011-2019 Sam C. Lin
 *
 * Open EVSE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.

 * Open EVSE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with Open EVSE; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "open_evse.h"

#ifndef taskENTER_CRITICAL
#define taskENTER_CRITICAL(x)
#endif
#ifndef taskEXIT_CRITICAL
#define taskEXIT_CRITICAL(x)
#endif

#ifdef portMUX_INITIALIZER_UNLOCKED
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#endif

#ifdef GFI
// interrupt service routing
void gfi_isr()
{
  g_EvseController.SetGfiTripped();
}


void Gfi::Init(uint8_t v6)
{
#ifndef OPEN_EVSE_LIB
  pin.init(GFI_REG,GFI_IDX,DigitalPin::INP);
  // GFI triggers on rising edge
  attachInterrupt(GFI_INTERRUPT,gfi_isr,RISING);

#ifdef GFI_SELFTEST
  volatile uint8_t *reg = GFITEST_REG;
  volatile uint8_t idx  = GFITEST_IDX;
#ifdef OEV6
  if (v6) {
    reg = V6_GFITEST_REG;
    idx  = V6_GFITEST_IDX;
  }
#endif // OEV6
  pinTest.init(reg,idx,DigitalPin::OUT);
#endif // GFI_SELFTEST

#else
  pin.init(NULL, ESP_TRIP, DigitalPin::PinMode::INP);
  attachInterrupt(ESP_TRIP, gfi_isr, RISING); // hook isr handler for specific gpio pin

#ifdef GFI_SELFTEST
  pinTest.init(NULL, ESP_STP, DigitalPin::PinMode::OUT);
  pinTest.write(0);
#endif

#ifdef ESP_CAL
  pinCal.init(NULL, ESP_CAL, DigitalPin::PinMode::OUT);
  pinCal.write(0);
#endif

  detachInterrupt(ESP_TRIP); // remove isr handler for gpio number
  attachInterrupt(ESP_TRIP, gfi_isr, RISING); // hook isr handler for specific gpio pin again
#endif
  Reset();
}

// NOTE: timeout in general
// generate a TEST current, timeout 1000 ms
// wait for GFI pin to clear, timeout 2000 ms

//RESET GFI LOGIC
void Gfi::Reset()
{
  WDT_RESET();

  taskENTER_CRITICAL(&mux);
#ifdef GFI_SELFTEST
  testInProgress = 0;
  testSuccess = 0;
#endif // GFI_SELFTEST

  if (pin.read()) m_GfiFault = 1; // if interrupt pin is high, set fault
  else m_GfiFault = 0;
  taskEXIT_CRITICAL(&mux);
}

void Gfi::SetFault()
{
  taskENTER_CRITICAL(&mux);
  m_GfiFault = 1;
  taskEXIT_CRITICAL(&mux);
}

uint8_t Gfi::Fault()
{
  // FIXME refactor this class and don't block all interrupts with
  // enter_critical all the time
  return m_GfiFault;
}

#ifdef GFI_SELFTEST

// return fault #:
//  0 = fault 0 (no fault, ok)
//  1 = fault 1 (irq no trip)
//  2 = fault 2 (pin not clear before selftest)
//  3 = fault 3 (pin not clear after selftest)
uint8_t Gfi::SelfTest()
{
  // Do we need to calibrate RCMB?
#if GFI_MODULE == RCMB_CT
  // CT does not have a CAL pin
#elif GFI_MODULE == RCMB_RCM14_01 || GFI_MODULE == RCMB_RCM14_03
  // Calibrate the RCM14-01/03
  // Please note: if a RCM14-01/03 is connected to EVCC5, the RCM14-01/03 don't have CAL pin, so our pinCal does not affect RCM14-01/03's work
  delay(130);       // T1 >= 100 ms
  pinCal.write(1);  // set HIGH to trigger RCM14-01/03's CAL pin to LOW to prepare calibration
  delay(70);        // 50 ms <= T2 <= 100 ms
  pinCal.write(0);  // set LOW again to trigger RCM14-01/03's CAL pin to HIGH to start calibration
  delay(500);       // after T3 >= 500 ms, ready to check FAULT OUT
#elif GFI_MODULE == RCMB_MC003E1_E1
  // Calibrate the RCMB_MC003E1_E1
  // EVCC6: ESP_CAL pin is connected to the G of a MOSFET, MC_CAL pin is connected to the pullup D and S is taild to ground
  delay(130);       // T1 >= 100 ms
  pinCal.write(1);  // set HIGH to tail MC_CAL pin to LOW level
  delay(70);        // 50 ms <= T2 <= 100 ms
  pinCal.write(0);  // set LOW to pullup MC_CAL pin to HIGH level again
  delay(500);       // after T3 >= 500 ms, ready to check FAULT OUT
#elif GFI_MODULE == RCMB_MC003E3_C1 || GFI_MODULE == RCMB_MC003E5_C1
  // MC003E3/5-C1's TEST-IN pin has calibration function
#endif
  int i;
  // wait for GFI pin to clear
  for (i=0;i < 20;i++) {
    WDT_RESET();
    if (!pin.read()) break;
    delay(50);
  }
  // is interrupt pin clear at self-test begin?
  DBUGF("GFI SelfTest: interrupt pin %s clear at begin", pin.read() ? "not" : "is");
  if (i == 20) return 2;

  taskENTER_CRITICAL(&mux);
  testInProgress = 1;
  testSuccess = 0;
  // FIXME clear fault before do self test
  m_GfiFault = 0;
  taskEXIT_CRITICAL(&mux);

  curms = millis();
#if GFI_MODULE == RCMB_CT
  // generate a TEST current, timeout 50 or 60 times each 2*(10 or 8.333) ms = 1000 ms
  // simulate an AC residual current wave with 50Hz or 60Hz for CT/RCM14-01/03 with a TEST coil
  // 1 sec. has 50 or 60 sinus waves
  // 1 positive halfwave has 1000000 us/50 or 60/2 = 10000 or 8333 us = 10 or 8.333 ms
  // ==> GFI_TEST_CYCLES=50 or 60, GFI_PULSE_ON_US=10000 or 8333, GFI_PULSE_OFF_US=10000 or 8334
  for (i=0; !SelfTestSuccess() && (i < GFI_TEST_CYCLES); i++) {
    pinTest.write(1);
    delayMicroseconds(GFI_PULSE_ON_US);
    pinTest.write(0);
    delayMicroseconds(GFI_PULSE_OFF_US);
  }

#elif GFI_MODULE == RCMB_RCM14_01 || GFI_MODULE == RCMB_RCM14_03
  // generate a TEST current
  // EVCC: set pinTest pin to HIGH to trigger RCM's TEST pin to HIGH level for RCM14-01/03 or MC003E1-E1
  pinTest.write(1);
  for (i=0; !SelfTestSuccess() && (i < 40); i++) {
    WDT_RESET();
    delay(50);
  }
  pinTest.write(0);

#elif GFI_MODULE == RCMB_MC003E1_E1
  // generate a TEST current
  // EVCC6: pinTest pin is via R330 directly connected to MC_STP pin
  pinTest.write(1);
  for (i=0; !SelfTestSuccess() && (i < 40); i++) {
    WDT_RESET();
    delay(50);
  }
  pinTest.write(0);

#elif GFI_MODULE == RCMB_MC003E3_C1 || GFI_MODULE == RCMB_MC003E5_C1
  // generate a TEST current
  // MC003E3/5-C1's TEST-IN pin is a calibration+test pin
  delay(130);       // T1 >= 100 ms
  pinTest.write(0); // set MC003E3/5-C1's TEST-IN pin to LOW to prepare calibration
  delay(70);        // wait 50 ms <= T2 <= 100 ms
  pinTest.write(1); // set MC003E3/5-C1's TEST-IN pin to HIGH to start calibration
  for (i=0; !SelfTestSuccess() && (i < 40); i++) {
    WDT_RESET();
    delay(50);
  }

#endif

  // is interrupt pin tripped in self-test?
  // reality: RCM14-01/03  T3 = 1200 ms
  // reality: MC003E3/5-C1 T3 = 270 ms
  // reality: MC003E1-E1 T3 = 43 ms
  DBUGF("GFI SelfTest: interrupt %s tripped, T3 = %ld ms", !SelfTestSuccess() ? "not" : "is", millis() - curms);
  if (!SelfTestSuccess()) return 1; // fault 1 (irq no trip)

  // wait for GFI pin to clear
  // MC003E3/5-C1 datasheet: IRQ pin HIGH duration should be max T6 <= 1400 ms
  curms = millis();
  for (i=0;i < 40;i++) {
    WDT_RESET();
    if (!pin.read()) break;
    delay(50);
  }
  // is interrupt pin clear at self-test end?
  // reality: RCM14-01/03  T6 = 300 ms
  // reality: MC003E3/5-C1 T6 = 1551 ms
  DBUGF("GFI SelfTest: interrupt pin %s clear again, T6 = %ld ms", pin.read() ? "not" : "is", millis() - curms);
  if (i == 40) return 3;

#if !defined(OPENEVSE_2) && !defined(OPEN_EVSE_LIB)
  // sometimes getting spurious GFI faults when testing just before closing
  // relay.
  // wait a little more for everything to settle down
  // this delay is needed only if 10uF cap is in the circuit, which makes the circuit
  // temporarily overly sensitive to trips until it discharges
  wdt_delay(1000);
#endif // OPENEVSE_2

  taskENTER_CRITICAL(&mux);
  m_GfiFault = 0; // clear fault after self test
  testInProgress = 0;
  taskEXIT_CRITICAL(&mux);

  DBUGF("GFI SelfTest %s", !SelfTestSuccess() ? "NOK" : "OK");
  return !SelfTestSuccess();
}

void Gfi::SetTestSuccess()
{
  taskENTER_CRITICAL(&mux);
  testSuccess = 1;
  taskEXIT_CRITICAL(&mux);
}

uint8_t Gfi::SelfTestSuccess()
{
  uint8_t rc;
  taskENTER_CRITICAL(&mux);
  rc = testSuccess;
  taskEXIT_CRITICAL(&mux);
  return rc;
}

uint8_t Gfi::SelfTestInProgress()
{
  uint8_t rc;
  taskENTER_CRITICAL(&mux);
  rc = testInProgress;
  taskEXIT_CRITICAL(&mux);
  return rc;
}
#endif // GFI_SELFTEST
#endif // GFI
