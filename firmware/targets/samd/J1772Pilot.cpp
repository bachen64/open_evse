#include "open_evse.h"
#include <wiring_private.h>

static constexpr uint32_t kPilotTop = 47999; // 48MHz / 1 / (47999 + 1) = 1kHz

void J1772Pilot::pwmEnable(bool tf)
{
  if (tf) {
  // configure PWM
  pinPeripheral(PILOT_REG, PIO_TIMER_ALT); // PA18 -> TCC0/WO[2]

  PM->APBCMASK.reg |= PM_APBCMASK_TCC0;

  GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN |
                                 GCLK_CLKCTRL_GEN_GCLK0 |
                                 GCLK_CLKCTRL_ID_TCC0_TCC1);
  while (GCLK->STATUS.bit.SYNCBUSY) {
  }

  TCC0->CTRLA.bit.ENABLE = 0;
  while (TCC0->SYNCBUSY.bit.ENABLE) {
  }

  TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1;
  TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
  while (TCC0->SYNCBUSY.bit.WAVE) {
  }

  TCC0->PER.reg = kPilotTop;
  while (TCC0->SYNCBUSY.bit.PER) {
  }

  TCC0->CC[2].reg = 0;
  while (TCC0->SYNCBUSY.bit.CC2) {
  }

  TCC0->CTRLA.bit.ENABLE = 1;
  while (TCC0->SYNCBUSY.bit.ENABLE) {
  }
  // end configure PWM
  }
  else {
    TCC0->CTRLA.bit.ENABLE = 0;
    while (TCC0->SYNCBUSY.bit.ENABLE) {
    }

    pinMode(PILOT_REG, OUTPUT);
  }
}

void J1772Pilot::pwmSetCompareCounts(uint32_t compare)
{
  if (compare > kPilotTop) {
    compare = kPilotTop;
  }

  TCC0->CC[2].reg = compare;
  while (TCC0->SYNCBUSY.bit.CC2) {
  }
}


void J1772Pilot::Init()
{
  pinMode(PILOT_REG,OUTPUT);



  SetState(PILOT_STATE_P12); // turns the pilot on 12V steady state
}


// no PWM pilot signal - steady state
// PILOT_STATE_P12 = steady +12V (EVSE_STATE_A - VEHICLE NOT CONNECTED)
// PILOT_STATE_N12 = steady -12V (EVSE_STATE_F - FAULT) 
void J1772Pilot::SetState(PILOT_STATE state)
{
  if (m_State == PILOT_STATE_PWM) pwmEnable(0);

  digitalWrite(PILOT_REG,(state == PILOT_STATE_P12) ? HIGH : LOW);

  m_State = state;
  //  g_serial->printf("pstate: %s\n",(state == PILOT_STATE_P12) ? "P12":"N12");
}

//
// set EVSE current capacity in Amperes
// 
// outputting a 1KHz square wave w/ proper duty cycle
// quick reference:
//   6A  -> 10%
//   16A -> 26.67%
//   32A -> 53.33%
//   51A -> 85%
//   52A -> 84.8%
//   80A -> 96%
//
int J1772Pilot::SetPWM(int amps)
{
  uint32_t compare;
  if ((amps >= 6) && (amps <= 51)) {
    // amps = (duty cycle %) X 0.6
    // duty% = amps / 0.6 = amps * 5 / 3
    // compare = duty% * (PER+1) / 100 = amps * 800 (exact integer)
    compare = (uint32_t)amps * 800u;
  } else if ((amps > 51) && (amps <= 80)) {
    // amps = (duty cycle % - 64) X 2.5
    // duty% = 64 + amps / 2.5 = 64 + 2*amps/5
    // compare = 64%*(PER+1) + amps*192 (exact integer)
    compare = 30720u + ((uint32_t)amps * 192u);
  }
  else {
    return 1;
  }

  pwmEnable(1);
  pwmSetCompareCounts(compare);

  m_State = PILOT_STATE_PWM;

  //  g_serial->printf("pwm cnt: %d\n",cnt);
  return 0;
}
