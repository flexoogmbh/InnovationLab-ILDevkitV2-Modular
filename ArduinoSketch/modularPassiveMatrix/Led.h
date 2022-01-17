#ifndef LED_H
#define LED_H

#include "Arduino.h"

enum class Color{
	OFF,
	GREEEN,
	RED,
	BLUE,
	YELLOW,
	MAGENTA,
	CYAN,
	WHITE
};

class Led {
public:
	Led(int8_t l_green, int8_t l_red, int8_t l_blue);
	~Led();
	bool set(Color l_color);
private:
	int8_t m_green;
	int8_t m_red;
	int8_t m_blue;
};

#endif // ADMUX_MUX_H
