#ifndef ADS7049_H
#define ADS7049_H

#include "Arduino.h"
#include "SPI.h"

class Ads7049 {
  public:
    Ads7049(int l_csPin, uint32_t l_speed);
    ~Ads7049();

    volatile bool begin();
    volatile bool end();
    bool powerOnCalibrate();
    bool operationCalibrate();
    bool getValue(uint16_t *l_pValue);
  private:
    int m_chipSelect;
    SPISettings m_spiSettings;
};

#endif
