#if defined(ENABLE_DEBUG) && !defined(ENABLE_DEBUG_ENERGY_METER)
#undef ENABLE_DEBUG
#endif

#include "EnergyMeter.h"

#ifdef KWH_RECORDING

AnalogEnergyMeter g_EnergyMeter;


AnalogEnergyMeter::AnalogEnergyMeter()
{
  m_bFlags = 0;
  m_wattSeconds = 0;

  // check for unitialized eeprom condition so it can begin at 0kWh
  if (eeprom_read_dword((uint32_t*)EOFS_KWH_ACCUMULATED) == 0xffffffff) {
    // set the four bytes to zero just once in the case of unitialized eeprom
    eeprom_write_dword((uint32_t*)EOFS_KWH_ACCUMULATED,0);
  }
  
  // get the stored value for the kWh from eeprom
  m_wattHoursTot = eeprom_read_dword((uint32_t*)EOFS_KWH_ACCUMULATED);
}

void AnalogEnergyMeter::Update()
{
  // 1. charging session begins when EV is connected, ends when EV disconnected
  // 2. we record only when the relay is closed
  // 3. total kWh updated only when session ends
  uint8_t evconnected = g_EvseController.EvConnected();

  if (!evConnected() && evconnected) {
    startSession();
  }
  else if (!evconnected && evConnected()) {
    endSession();
  }

  if (inSession()) {
    uint8_t relayclosed = g_EvseController.RelayIsClosed();

    if (relayclosed) {
      if (!relayClosed()) {
	// relay just closed - don't calc, just reset timer
	m_lastUpdateMs = millis();
      }
      else {
	calcUsage();
      }
    }

    if (relayclosed) setRelayClosed();
    else clrRelayClosed();
  }

  if (evconnected) setEvConnected();
  else clrEvConnected();
}

void AnalogEnergyMeter::calcUsage()
{
  unsigned long curms = millis();
  unsigned long dms = curms - m_lastUpdateMs;
  if (dms > KWH_CALC_INTERVAL_MS) {
#ifndef FAKE_CHARGING_CURRENT
      uint32_t mv = g_EvseController.GetVoltage();
      uint32_t ma = g_EvseController.GetChargingCurrent();
      /*
       * The straightforward formula to compute 'milliwatt-seconds' would be:
       *     mws = (mv/1000) * (ma/1000) * dms;
       *
       * However, this has some serious drawbacks, namely, truncating values
       * when using integer math. This can introduce a lot of error!
       *     5900 milliamps -> 5.9 amps (float) -> 5 amps (integer)
       *     0.9 amps difference / 5.9 amps -> 15.2% error
       *
       * The crazy equation below helps ensure our intermediate results always
       * fit in a 32-bit unsigned integer, but retain as much precision as
       * possible throughout the calculation. Here is how it was derived:
       *     mws = (mv/1000) * (ma/1000) * dms;
       *     mws = (mv/(2**3 * 5**3)) * (ma/(2**3 * 5**3)) * dms;
       *     mws = (mv/2**3) * (ma/(2**3) / 5**6 * dms;
       *     mws = (mv/2**4) * (ma/(2**2) / 5**6 * dms;
       *
       * By breaking 1000 into prime factors of 2 and 5, and shifting things
       * around, we almost match the precision of floating-point math.
       *
       * Using 16 and 4 divisors, rather than 8 and 8, helps precision because
       * mv is always greater than ~100000, but ma can be as low as ~6000.
       *
       * A final note- the divisions by factors of 2 are done with right shifts
       * by the compiler, so the revised equation, although it looks quite
       * complex, only requires one divide operation.
       */
      uint32_t mws = (mv/16) * (ma/4) / 15625 * dms;
#ifdef THREEPHASE
      // Multiply calculation by 3 to get 3-phase energy.
      // Typically you'd multiply by sqrt(3), but because voltage is measured to
      // ground (230V) rather than between phases (400 V), 3 is the correct multiple.
      mws *= 3;
#endif // THREEPHASE
      // convert milliwatt-seconds to watt-seconds and increment counter
      m_wattSeconds += mws / 1000;
#endif // FAKE_CHARGING_CURRENT
      m_lastUpdateMs = curms;
  }
}

void AnalogEnergyMeter::startSession()
{
  endSession();
  m_wattSeconds = 0;
  m_lastUpdateMs = millis();
  setInSession();
}

void AnalogEnergyMeter::endSession()
{
  if (inSession()) {
    clrInSession();
    if (m_wattSeconds) {
    m_wattHoursTot += (m_wattSeconds / 3600UL);
      SaveTotkWh();
    }
  }
}

void AnalogEnergyMeter::SaveTotkWh()
{
  // only write to EEPROM if changed
  if (eeprom_read_dword((uint32_t*)EOFS_KWH_ACCUMULATED) != m_wattHoursTot) {
    eeprom_write_dword((uint32_t*)EOFS_KWH_ACCUMULATED,m_wattHoursTot);
    DBUG("Write to EEPROM "); DBUGVAR(m_wattHoursTot);
  }
}

void AnalogEnergyMeter::SetTotkWh(uint32_t whtot)
{
  // 2. we record only when the EM set energy total in Wh
  if (inSession()) { // calc usage when EV is connected (in session)
    m_wattSeconds = (whtot - m_wattHoursTot) * 3600;
    DBUG("Calc "); DBUGVAR(m_wattSeconds);
  }
  else { // sync total kWh when EV not connected (not in session)
    m_wattHoursTot = whtot;
    DBUG("Sync "); DBUGVAR(m_wattHoursTot);
  }
}

uint32_t AnalogEnergyMeter::GetTotkWh()
{
  // check again for unitialized eeprom condition so it can begin at 0kWh
  if (eeprom_read_dword((uint32_t*)EOFS_KWH_ACCUMULATED) == 0xffffffff) {
    // set the four bytes to zero just once in the case of unitialized eeprom
    eeprom_write_dword((uint32_t*)EOFS_KWH_ACCUMULATED,0);
  }

  // if needed, get again the stored value for the kWh from eeprom
  if (!m_wattHoursTot) {
    m_wattHoursTot = eeprom_read_dword((uint32_t*)EOFS_KWH_ACCUMULATED);
    DBUG("Read from EEPROM "); DBUGVAR(m_wattHoursTot);
  }

  return m_wattHoursTot;
}

#endif // KWH_RECORDING

