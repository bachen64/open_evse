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

#ifdef GFI
// interrupt service routing
void gfi_isr()
{
  g_EvseController.SetGfiTripped();
}


void Gfi::Init(uint8_t v6)
{
  pin.init(GFI_REG,GFI_IDX,DigitalPin::INP);
  // GFI triggers on rising edge
  attachInterrupt(digitalPinToInterrupt(GFI_REG),gfi_isr,RISING);

#ifdef GFI_SELFTEST
  uint32_t reg = GFITEST_REG;
  uint32_t idx  = GFITEST_IDX;

  pinTest.init(reg,idx,DigitalPin::OUT);
#endif // GFI_SELFTEST

  Reset();
}


//RESET GFI LOGIC
void Gfi::Reset()
{
  WDT_RESET();

#ifdef GFI_SELFTEST
  testInProgress = 0;
  testSuccess = 0;
#endif // GFI_SELFTEST

  if (pin.read()) m_GfiFault = 1; // if interrupt pin is high, set fault
  else m_GfiFault = 0;
}

#ifdef GFI_SELFTEST

uint8_t Gfi::SelfTest()
{
  int i;
  // wait for GFI pin to clear
  for (i=0;i < 20;i++) {
    WDT_RESET();
    if (!pin.read()) break;
    delay(50);
  }
  if (i == 20) return 2;

  testInProgress = 1;
  testSuccess = 0;
  WDT_RESET();
  //  uint32_t sms = millis();
  for(int i=0; !testSuccess && (i < 200); i++) {
    pinTest.write(1);
    delayMicroseconds(GFI_PULSE_ON_US);
    pinTest.write(0);
    delayMicroseconds(GFI_PULSE_OFF_US);
  }
  //  RAPI_SERIAL_PORT.print("GFI ");RAPI_SERIAL_PORT.println(millis()-sms);

  // wait for GFI pin to clear
  for (i=0;i < 40;i++) {
    WDT_RESET();
    if (!pin.read()) break;
    delay(50);
  }
  if (i == 40) return 3;


#ifdef dontneed
  // sometimes getting spurious GFI faults when testing just before closing
  // relay.
  // wait a little more for everything to settle down
  // this delay is needed only if 10uF cap is in the circuit, which makes the circuit
  // temporarily overly sensitive to trips until it discharges
  wdt_delay(1000);
#endif 

  m_GfiFault = 0;
  testInProgress = 0;

  return !testSuccess;
}
#endif // GFI_SELFTEST
#endif // GFI
