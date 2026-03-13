#include "open_evse.h"

//                                               A/B  B/C  C/D  D DS
THRESH_DATA J1772EVSEController::m_ThreshData = {3948,3539,3258,0,492};

#ifdef MCU_ID_LEN
// mcuid *must* be of size MCU_ID_LEN
void getMcuId(uint8_t *mcuid)
{
  for (uint8_t word = 0; word < 4; word++) {
    uint32_t value;
    switch (word) {
      case 0:
        value = *((volatile uint32_t *)0x0080A00C);
        break;
      case 1:
        value = *((volatile uint32_t *)0x0080A040);
        break;
      case 2:
        value = *((volatile uint32_t *)0x0080A044);
        break;
      default:
        value = *((volatile uint32_t *)0x0080A048);
        break;
    }
    mcuid[(word * 4) + 0] = (uint8_t)(value & 0xFF);
    mcuid[(word * 4) + 1] = (uint8_t)((value >> 8) & 0xFF);
    mcuid[(word * 4) + 2] = (uint8_t)((value >> 16) & 0xFF);
    mcuid[(word * 4) + 3] = (uint8_t)((value >> 24) & 0xFF);
  }
}
#endif // MCU_ID_LEN


void DigitalPin::init(uint32_t pinnum,int idxjunk,PinMode mode)
{
  _pinNum = pinnum;
  if (mode == INP) {
    _pinMode = INPUT;
  }
  else if (mode == INP_PU) {
    _pinMode = INPUT_PULLUP;
  }
  else {
    _pinMode = OUTPUT;
  }
  pinMode(_pinNum,_pinMode);
}


// platform-specific init
void initTarget()
{
  //n.b. set BOD via fuses, not this code, as fuses may lock out BOD from being
  // manipulated in code, anyway
  // default factory fuse setting is 1.78V BOD enabled
#ifdef DONTUSE
    // 1. Prepare the configuration value level 7 = ~1.78V
    uint32_t bod33_config = SYSCTRL_BOD33_LEVEL(7)     | 
                            SYSCTRL_BOD33_HYST      | 
                            SYSCTRL_BOD33_ACTION_RESET | 
                            SYSCTRL_BOD33_ENABLE;

    // 2. Wait for the SYSCTRL to be ready/synchronized 
    // (BOD33 is often in a different clock domain)
    while (SYSCTRL->PCLKSR.bit.BOD33RDY == 0);

    // 3. Disable before configuring if it was already running 
    // (Optional, but ensures Level change takes effect cleanly)
    SYSCTRL->BOD33.bit.ENABLE = 0;
    while (SYSCTRL->PCLKSR.bit.BOD33RDY == 0);

    // 4. Write the full configuration
    SYSCTRL->BOD33.reg = bod33_config;

    // 5. Wait for synchronization again to ensure it's active
    while (SYSCTRL->PCLKSR.bit.BOD33RDY == 0);
#endif // DONTUSE
}
