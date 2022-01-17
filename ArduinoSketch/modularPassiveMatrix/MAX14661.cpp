#include "MAX14661.h"

MAX14661::MAX14661(pin_size_t l_chipSelect, pin_size_t l_shutdown, uint16_t l_chainLength, uint32_t l_speed, uint32_t l_switchDelay)
{
  m_chipSelect = l_chipSelect;
  m_shutdown = l_shutdown;

  m_chainLength = l_chainLength;
  m_regNum = m_chainLength * 4;
  m_maxChnlNum = m_regNum * 8;
  m_switchDelay = l_switchDelay;

  m_pData = new Max14661DataContainer[l_chainLength];

  pinMode(m_chipSelect, OUTPUT);
  pinMode(m_shutdown, OUTPUT);

  digitalWrite(m_chipSelect, HIGH);

  m_spiSettings = SPISettings(l_speed, MSBFIRST, SPI_MODE0);

  disable();
}

MAX14661::~MAX14661()
{
  delete[] m_pData;
}

volatile bool MAX14661::begin()
{
  SPI.beginTransaction(m_spiSettings);
  return true;
}

volatile bool MAX14661::end()
{
    SPI.endTransaction();
    return true;
}

/**
 * @brief  Set OE pin to 0
 * @param  none
 * @retval none
 */
bool MAX14661::enable()
{
  digitalWrite(m_shutdown, HIGH);
  return true;
}

/**
 * @brief  Set OE pin to 1
 * @param  none
 * @retval none
 */
bool MAX14661::disable()
{
  digitalWrite(m_shutdown, LOW);
  return true;
}

/**
 * @brief  Filling an array by zero
 * @param  length: length an array of register
 * @retval none
 */
bool MAX14661::reset()
{
  resetBuffer();

  digitalWrite(m_chipSelect, HIGH);

  return setData();
}
/**
 * @brief  Send high level
 * @param  value: the data than will be sended
 * @retval none
 */
bool MAX14661::switchChnlA(uint16_t l_chnl)
{
  bool l_result = true;

  if (l_chnl >= (m_maxChnlNum / 2))
  {
    return false;
  }

  m_activeChnlA = l_chnl;

  uint16_t l_register = (1 << (m_activeChnlA % 16));
  m_pData[m_chainLength - (1 + l_chnl / 16)].uint8_[3] = l_register & 0x00FF;
  m_pData[m_chainLength - (1 + l_chnl / 16)].uint8_[2] = l_register >> 8;

  m_pData[m_chainLength - (1 + l_chnl / 16)].uint16_[0] =
      ~m_pData[m_chainLength - (1 + l_chnl / 16)].uint16_[1];

  l_result = setData();

  delayMicroseconds(m_switchDelay);

  return l_result;
}

bool MAX14661::switchChnlB(uint16_t l_chnl)
{
  bool l_result = true;

  if (l_chnl >= (m_maxChnlNum / 2))
  {
    return false;
  }

  m_activeChnlB = l_chnl;

  uint16_t l_register = (1 << (m_activeChnlB % 16));
  m_pData[m_chainLength - (1 + l_chnl / 16)].uint8_[1] = l_register & 0x00FF;
  m_pData[m_chainLength - (1 + l_chnl / 16)].uint8_[0] = l_register >> 8;
  m_pData[m_chainLength - (1 + l_chnl / 16)].uint16_[1] =
      ~m_pData[m_chainLength - (1 + l_chnl / 16)].uint16_[0];

  l_result = setData();

  delayMicroseconds(m_switchDelay);

  return l_result;
}

/**
 * @brief  Send low level
 * @param  value: the data than will be sended
 * @retval none
 */
bool MAX14661::nextChnlA()
{
  ++m_activeChnlA;

  if (m_activeChnlA >= (m_maxChnlNum / 2))
  {
    m_activeChnlA = 0;
  }

  return switchChnlA(m_activeChnlA);
}

bool MAX14661::nextChnlB()
{
  ++m_activeChnlB;

  if (m_activeChnlB >= (m_maxChnlNum / 2))
  {
    m_activeChnlB = 0;
  }

  return switchChnlB(m_activeChnlB);
}

void MAX14661::resetBuffer()
{
  for (uint8_t i = 0; i < m_chainLength; i++)
  {
    m_pData[i].uint32_ = 0;
  }
}

bool MAX14661::setAllToA()
{
  for (uint8_t i = 0; i < m_chainLength; i++)
  {
    m_pData[i].uint8_[0] = 0;
    m_pData[i].uint8_[1] = 0;
    m_pData[i].uint8_[2] = 0xFF;
    m_pData[i].uint8_[3] = 0xFF;
  }

  return true;
}

bool MAX14661::setAllToB()
{
  for (uint8_t i = 0; i < m_chainLength; i++)
  {
    m_pData[i].uint8_[0] = 0xFF;
    m_pData[i].uint8_[1] = 0xFF;
    m_pData[i].uint8_[2] = 0;
    m_pData[i].uint8_[3] = 0;
  }

  return true;
}

bool MAX14661::switchAllToA()
{
  setAllToA();
  return setData();
}

bool MAX14661::switchAllToB()
{
  setAllToB();
  return setData();
}

bool MAX14661::setData()
{
  begin();
  
  digitalWrite(m_chipSelect, LOW);

  SPI.transfer(m_pData[0].uint8_, m_regNum);

  digitalWrite(m_chipSelect, HIGH);

  end();

  return true;
}