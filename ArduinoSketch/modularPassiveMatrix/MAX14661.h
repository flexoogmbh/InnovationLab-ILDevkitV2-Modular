#pragma once

#include "Arduino.h"
#include "SPI.h"
#include "PinNames.h"

#define MAX14661_LIB_VERSION (F("0.1.0"))

union Max14661DataContainer
{
  uint32_t uint32_;
  uint16_t uint16_[2];
  uint8_t uint8_[4];
};

class MAX14661
{
public:
  MAX14661(pin_size_t l_chipSelect, pin_size_t l_shutdown, uint16_t l_chainLength, uint32_t l_speed, uint32_t l_switchDelay);
  ~MAX14661();

  volatile bool begin();
  volatile bool end();
  bool enable();
  bool disable();
  bool reset(); // Unselect all columns
  bool switchChnlA(uint16_t l_chnl);
  bool switchChnlB(uint16_t l_chnl);
  bool nextChnlA();
  bool nextChnlB();
  bool setAllToA();
  bool setAllToB();
  bool switchAllToA();
  bool switchAllToB();

  int lastError();

private:
  pin_size_t m_chipSelect;
  pin_size_t m_shutdown;
  uint32_t m_switchDelay;

  Max14661DataContainer *m_pData;
  uint16_t m_chainLength;
  uint16_t m_regNum;

  uint16_t m_activeChnlA;
  uint16_t m_activeChnlB;
  uint16_t m_maxChnlNum;

  SPISettings m_spiSettings;

  int m_error;

  void resetBuffer();
  bool setData();
};

// -- END OF FILE --
