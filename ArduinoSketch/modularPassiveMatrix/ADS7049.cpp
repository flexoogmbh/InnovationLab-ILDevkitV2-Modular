#include "ADS7049.h"

Ads7049::Ads7049(int l_csPin, uint32_t l_speed)
{
  m_chipSelect = l_csPin;
  pinMode(m_chipSelect, OUTPUT);

  m_spiSettings = SPISettings(l_speed, MSBFIRST, SPI_MODE0);

  powerOnCalibrate();
}

Ads7049::~Ads7049()
{
}

volatile bool Ads7049::begin()
{
  SPI.beginTransaction(m_spiSettings);
  return true;
}

volatile bool Ads7049::end()
{
	SPI.endTransaction();
	return true;
}

bool Ads7049::powerOnCalibrate()
{
	begin();
	digitalWrite(m_chipSelect, LOW); //Enable ADC chip select
	SPI.transfer(0);                 //Make 16 clock pulses for calibration
	SPI.transfer(0);
	digitalWrite(m_chipSelect, HIGH); //Disable ADC communication
	end();
  return true;
}

bool Ads7049::operationCalibrate()
{
	begin();

	digitalWrite(m_chipSelect, LOW); //Enable ADC chip select
	SPI.transfer(0);                 //Make 32 clock pulses for calibration
	SPI.transfer(0);
	SPI.transfer(0);
	SPI.transfer(0);
	digitalWrite(m_chipSelect, HIGH); //Disable ADC communication

	end();

	return true;
}

bool Ads7049::getValue(uint16_t *l_pValue)
{
	union Data {
		uint16_t l_word = 0;
		uint8_t l_byte[2];
	} l_data;

	begin();

	digitalWrite(m_chipSelect, LOW); //Enable ADC chip select
	l_data.l_byte[1] = SPI.transfer(0);
	l_data.l_byte[0] = SPI.transfer(0);
	
	digitalWrite(m_chipSelect, HIGH); //Disable ADC communication

	end();

	*l_pValue = l_data.l_word >> 2;

	return true;
}
