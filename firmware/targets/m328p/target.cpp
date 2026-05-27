#include "open_evse.h"

// wdt_init turns off the watchdog timer after we use it
// to reboot

#ifdef MCU_ID_LEN
// mcuid *must* be of size MCU_ID_LEN
#include <avr/boot.h>
void getMcuId(uint8_t *mcuid)
{
  for (int i=0;i < MCU_ID_LEN;i++) {
    mcuid[i] = boot_signature_byte_get(0x0E + i);
  }
}
#endif // MCU_ID_LEN


void wdt_init(void) __attribute__((naked,used)) __attribute__((section(".init3")));
void wdt_init(void)
{
  MCUSR = 0;
  wdt_disable();

  return;
}

//                                               A/B B/C C/D D DS
THRESH_DATA J1772EVSEController::m_ThreshData = {875,780,690,0,260};


void DigitalPin::init(volatile uint8_t* _reg,uint8_t idx,PinMode _mode)
{
  reg = _reg;
  bit = 1 << idx;
  mode(_mode);
}

void DigitalPin::mode(PinMode mode)
{
  switch(mode) {
  case INP_PU:
    *port() |= bit;
    // fall through
  case INP:
    *ddr() &= ~bit;
    break;
  default: // OUT
    *ddr() |= bit;
  }
}

//
// AdcPin
// 
#include "wiring_private.h"
#include "pins_arduino.h"

uint8_t AdcPin::refMode = DEFAULT;

void AdcPin::init(uint8_t _adcNum)
{
  channel = _adcNum;
#if defined(analogChannelToChannel)
 #if defined(__AVR_ATmega32U4__)
  if (channel >= 18) channel -= 18; // allow for pin or channel numbers
 #endif
  channel = analogChannelToChannel(channel);
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if (channel >= 54) channel -= 54; // allow for pin or channel numbers
#elif defined(__AVR_ATmega32U4__)
  if (channel >= 18) channel -= 18; // allow for pin or channel numbers
#elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
  if (channel >= 24) channel -= 24; // allow for pin or channel numbers
#else
  if (channel >= 14) channel -= 14; // allow for pin or channel numbers
#endif
}

uint16_t AdcPin::read()
{
  uint8_t low, high;
  
  
#if defined(ADCSRB) && defined(MUX5)
  // the MUX5 bit of ADCSRB selects whether we're reading from channels
  // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
  ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((channel >> 3) & 0x01) << MUX5);
#endif
  
  // set the analog reference (high two bits of ADMUX) and select the
  // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
  // to 0 (the default).
#if defined(ADMUX)
  ADMUX = (refMode << 6) | (channel & 0x07);
#endif
  
  // without a delay, we seem to read from the wrong channel
  //delay(1);
  
#if defined(ADCSRA) && defined(ADCL)
  // start the conversion
  sbi(ADCSRA, ADSC);
  
  // ADSC is cleared when the conversion finishes
  while (bit_is_set(ADCSRA, ADSC));
  
  // we have to read ADCL first; doing so locks both ADCL
  // and ADCH until ADCH is read.  reading ADCL second would
  // cause the results of each conversion to be discarded,
  // as ADCL and ADCH would be locked when it completed.
  low  = ADCL;
  high = ADCH;
#else
  // we dont have an ADC, return 0
  low  = 0;
  high = 0;
#endif
  
  // combine the two bytes
  return (high << 8) | low;
}


// platform-specific init
void initTarget()
{

/* detect the board
 * Board ID
PD7 | ADC6 |
  0 | 0 | OpenEVSE v1-v5
  1 | 0 | OEV6 v5.5
  0 | 1 | n/a
  1 | 1 | OEV6 + CGMI v6.5+
  *
  N.B. lincomatic's BETA V6 board inverts PD7 so V6 is PD7=0
 */
  DigitalPin pinPD7;
  pinPD7.init(&PIND,7,DigitalPin::INP);
  int pd7 = pinPD7.read();

  // ADC - fake as a digital pin
  int adc6 = analogRead(A6);
  if (adc6 > 512) adc6 = 1;
  else adc6 = 0;

  if (pd7 && !adc6) {
    g_isV6 = true;
    g_hasCGMI = false;
  }
  else if (pd7 && adc6) {
    g_isV6 = true;
    g_hasCGMI = true;
  }
  else {
    g_isV6 = false;
    g_hasCGMI = false;
  }

  extern char g_sTmp[];
  sprintf(g_sTmp,"isV6: %c  hasCGMI %c",g_isV6 ? '1':'0',g_hasCGMI ? '1':'0');
  Serial.println(g_sTmp);

}
