#include "ADG732.h"

using namespace admux;

Mux::Mux(Pinset l_channelPins, int8_t l_enablePin, int8_t l_csPin, int8_t l_wrPin) :
    m_channelPins(l_channelPins), m_enablePin(l_enablePin), m_csPin(l_csPin), m_wrPin(l_wrPin){
  
    for (int i = 0; i < m_channelPins.size(); i++) {
    pinMode(m_channelPins[i], OUTPUT);
  }

  m_channelCount = 1 << m_channelPins.size();
  m_channel = -1;

  if (isDefined(m_enablePin)) {
    pinMode(m_enablePin, OUTPUT);
  }

  if (isDefined(m_wrPin)) {
      pinMode(m_wrPin, OUTPUT);
  }

  if (isDefined(m_csPin)) {
      pinMode(m_csPin, OUTPUT);
  }

}

int8_t Mux::channel(int8_t value) {
    int8_t l_rVal = ERROR_SUCCESS;
    enable();
    cs(LOW);

    if (value == m_channel){
        return ERROR_SUCCESS;
    }else if (value >= m_channelCount) {
        return ERROR_OUT_OF_RANGE;
    }

    m_channel = value;
    
    for (uint8_t i = 0; i < m_channelPins.size(); i++) {
        digitalWrite(m_channelPins[i], bitRead(value, i));
    }

    write(LOW);
    delayMicroseconds(10);
    write(HIGH);
    

    return ERROR_SUCCESS;
}

int8_t Mux::enable()
{
    if (!isDefined(m_enablePin))
        return ERROR_UNDEFINED_PIN;

    m_enabled = true;

    digitalWrite(m_enablePin, LOW);

    return ERROR_SUCCESS;
}

int8_t Mux::disable()
{
    if (!isDefined(m_enablePin))
        return ERROR_UNDEFINED_PIN;

    m_enabled = false;

    digitalWrite(m_enablePin, HIGH);

    return ERROR_SUCCESS;
}

int8_t Mux::write(uint8_t l_value)
{
    if (!isDefined(m_wrPin))
        return ERROR_UNDEFINED_PIN;

    digitalWrite(m_wrPin, (l_value & 0x01));
    return ERROR_SUCCESS;
}

int8_t Mux::cs(uint8_t l_value)
{
    if (!isDefined(m_csPin))
        return ERROR_UNDEFINED_PIN;

    digitalWrite(m_csPin, (l_value & 0x01));
    return ERROR_SUCCESS;
}