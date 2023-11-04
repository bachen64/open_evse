/*
 * Open EVSE Firmware
 *
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
#pragma once

#include "avrstuff.h"

// GFI module
#define RCMB_CT             0
#define RCMB_RCM14_01       1
#define RCMB_RCM14_03       2
#define RCMB_MC003E1_E1     3
#define RCMB_MC003E3_C1     4
#define RCMB_MC003E5_C1     5

#ifndef GFI_MODULE
#define GFI_MODULE          RCMB_CT
#endif

class Gfi {
  DigitalPin pin;
#ifdef GFI_SELFTEST
  DigitalPin pinTest;
#endif // GFI_SELFTEST
#ifdef ESP_CAL
  DigitalPin pinCal;
#endif
  uint8_t m_GfiFault;
#ifdef GFI_SELFTEST
  uint8_t testSuccess;
  uint8_t testInProgress;
#endif // GFI_SELFTEST
  unsigned long curms;
public:
  Gfi()
    : m_GfiFault(0)
#ifdef GFI_SELFTEST
    , testSuccess(0)
    , testInProgress(0)
#endif
    , curms(0) {}

  void Init(uint8_t v6=0);
  void Reset();
  void SetFault();
  uint8_t Fault();
#ifdef GFI_SELFTEST
  uint8_t SelfTest();
  void SetTestSuccess();
  uint8_t SelfTestSuccess();
  uint8_t SelfTestInProgress();
#endif
};
