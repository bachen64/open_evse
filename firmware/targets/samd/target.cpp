#include "open_evse.h"

//                                               A/B  B/C  C/D  D DS
THRESH_DATA J1772EVSEController::m_ThreshData = {3948,3539,3258,0,492};

#ifdef MCU_ID_LEN
// mcuid *must* be of size MCU_ID_LEN
void getMcuId(uint8_t *mcuid)
{
  uint8_t uid[16];
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
    uid[(word * 4) + 0] = (uint8_t)(value & 0xFF);
    uid[(word * 4) + 1] = (uint8_t)((value >> 8) & 0xFF);
    uid[(word * 4) + 2] = (uint8_t)((value >> 16) & 0xFF);
    uid[(word * 4) + 3] = (uint8_t)((value >> 24) & 0xFF);
  }

  for (uint8_t i = 0; i < MCU_ID_LEN; i++) {
    mcuid[i] = uid[i % sizeof(uid)];
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
  // enable BOD - brown out detection - to prevent lockups when the voltage drops too low

  uint32_t bod33 = SYSCTRL->BOD33.reg;

  bod33 |= SYSCTRL_BOD33_ENABLE;
  bod33 |= SYSCTRL_BOD33_HYST;
  bod33 |= SYSCTRL_BOD33_CEN;
  bod33 &= ~SYSCTRL_BOD33_ACTION_Msk;
  bod33 |= SYSCTRL_BOD33_ACTION_RESET;

  SYSCTRL->BOD33.reg = bod33;
}
