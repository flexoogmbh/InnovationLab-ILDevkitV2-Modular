#ifndef ADMUX_MUX_H
#define ADMUX_MUX_H

#include "Arduino.h"

namespace admux {

const int8_t UNDEFINED = -1;

inline bool isDefined(int8_t value) {
  return value > UNDEFINED;
}

enum {
  ERROR_SUCCESS = 0,
  ERROR_WRONG_SIGNAL_MODE = -1,
  ERROR_UNHANDLED_OPERATION = -2,
  ERROR_OUT_OF_RANGE = -3,
  ERROR_UNDEFINED_PIN = -4
};


typedef struct Pinset {
  Pinset(
      uint8_t pin0,
      int8_t pin1 = UNDEFINED,
      int8_t pin2 = UNDEFINED,
      int8_t pin3 = UNDEFINED,
      int8_t pin4 = UNDEFINED,
      int8_t pin5 = UNDEFINED,
      int8_t pin6 = UNDEFINED,
      int8_t pin7 = UNDEFINED
      ){
    pins[0] = pin0;
    pins[1] = pin1;
    pins[2] = pin2;
    pins[3] = pin3;
    pins[4] = pin4;
    pins[5] = pin5;
    pins[6] = pin6;
    pins[7] = pin7;
    for(int i = 0; i < MAX_SIZE; i++) {
      if (!isDefined(pins[i])) {
        m_size = i;
        return;
      }
    }
    m_size = MAX_SIZE;
  }

  int8_t operator[](uint8_t index) {
    return pins[index];
  }

  uint8_t size() {
    return m_size;
  }

private:
  static const uint8_t MAX_SIZE = 8;

  int8_t pins[MAX_SIZE];
  uint8_t m_size;
} Pinset;

class Mux {
public:
  Mux(Pinset l_channelPins,
      int8_t l_enablePin = UNDEFINED,
      int8_t l_wrPin = UNDEFINED,
      int8_t l_csPin = UNDEFINED);
  int8_t channel(int8_t);
  int8_t enable();
  int8_t disable();
  int8_t write(uint8_t l_value);
  int8_t cs(uint8_t l_value);
protected:
  Pinset m_channelPins;
  int8_t m_enablePin = UNDEFINED;
  int8_t m_wrPin = UNDEFINED;
  int8_t m_csPin = UNDEFINED;

  bool m_enabled;
  uint8_t m_channelCount;
  uint8_t m_channel;
};

}

#endif // ADMUX_MUX_H
