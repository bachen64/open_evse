#if defined(ENABLE_DEBUG) && !defined(ENABLE_DEBUG_ACCCONTROLLER)
#undef ENABLE_DEBUG
#endif

// Copyright (C) 2016 Sam C. Lin
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

#include "open_evse.h"
/*
    The Auto Current Capacity Controller is based on a voltage divider

    Voltage Input (Vin)     5 V
    Resistance 1 (R1)       1K Ohm
    Resistance 2 (R2)       1.5K Ohm    680 Ohm     220 Ohm     100 Ohm
    Output Voltage (Vout)   3 V         2 V         0.9 V       0.45 V
    IEC PP Amps             13 A        20 A        32 A        63 A

    12BIT3V3:               3723        2482        1119        563
    adcVal:    [4095..3909........3103........1801........841........282...0]

    12BIT5V:                2457        1638        739         372
    adcVal:    [4095..3276........2048........1189........556........186...0]

    10BIT5V:                614         409         185         93
    adcVal:    [1023...819.........512.........297........139.........47...0]

    WT32-ETH01 PP_READ schematics:
        3V3
         |
        R1=330
         |
         +--10K-----IO39
         |
        R2=1K5|680|220|100
         |
        GND
    measurement:            3238        2624        1535        863
    adcVal:    [4095..3600........2800........2000........1200.......400...0]
*/
#ifdef PP_AUTO_AMPACITY

#ifdef PP_READ
static PP_AMPS s_ppAmps[] = {
  // ADC   A
  {    0, 16}, // default to 16A when PP resistor 0 Ohm
#if defined(ESP32)
  {  400, 63}, // 100 Ohm
  { 1200, 32}, // 220 Ohm
  { 2000, 20}, // 680 Ohm
  { 2800, 13}, // 1.5 KOhm
  { 3600,  0}  // when 3600 < adcval < 4095 then invalid PP, so 0 amps
#elif defined(__LGT8F__)
  {  186, 63}, // 100 Ohm
  {  556, 32}, // 220 Ohm
  { 1189, 20}, // 680 Ohm     
  { 2048, 13}, // 1.5 KOhm
  { 3276,  0}  // when 3276 < adcval < 4095 then invalid PP, so 0 amps
#elif defined(__AVR__)
  {   47, 63}, // 100 Ohm
  {  139, 32}, // 220 Ohm
  {  297, 20}, // 680 Ohm     
  {  512, 13}, // 1.5 KOhm
  {  819,  0}  // when 819 < adcval < 1023 then invalid PP, so 0 amps
#else
  {   93, 63}, // 100 Ohm  = 93
  {  185, 32}, // 220 Ohm  = 185
  {  415, 20}, // 680 Ohm  = 415
  {  615, 13}, // 1.5 KOhm = 615
  { 1023,  0}  // when 1023 < adcval then invalid PP, so 0 amps
#endif
};
#endif

AutoCurrentCapacityController::AutoCurrentCapacityController() :
  adcPP(PP_PIN)
{
}

uint8_t AutoCurrentCapacityController::ReadPPMaxAmps() // return 0 if disabled
{
  uint8_t amps = 0; // invalid PP

#ifdef PP_READ
  // n.b. should probably sample a few times and average it
  uint16_t adcval = adcPP.read();
  for (uint8_t i=1;i < sizeof(s_ppAmps)/sizeof(s_ppAmps[0]);i++) {
    if (adcval < s_ppAmps[i].adcVal) {
      amps = s_ppAmps[i-1].amps;
      break;
    }
  }

  //  Serial.print("pp: ");Serial.print(adcval);Serial.print(" amps: ");Serial.println(amps);
  DBUGF("PP %d %d", adcval, amps);
#else
  amps = eeprom_read_byte((uint8_t*)EOFS_PP_AMPS);
  if (amps == (uint8_t)0xff) {
    amps = 0;
  }
#endif

  ppMaxAmps = amps;
  return amps;
}


uint8_t AutoCurrentCapacityController::AutoSetCurrentCapacity()
{
  uint8_t amps = ReadPPMaxAmps();

  if (amps) {
    if (g_EvseController.GetCurrentCapacity() > ppMaxAmps) {
      g_EvseController.SetCurrentCapacity(ppMaxAmps,1,1);
    }
    return 0;
  }
  else {
    return 1;
  }
}

#endif //PP_AUTO_AMPACITY


