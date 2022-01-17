#include "Led.h"

Led::Led(int8_t l_green, int8_t l_red, int8_t l_blue)
{
	m_green = l_green;
	m_red = l_red;
	m_blue = l_blue;

	pinMode(m_green, OUTPUT);
	pinMode(m_red, OUTPUT);
	pinMode(m_blue, OUTPUT);

	set(Color::OFF);
}

Led::~Led()
{

}

bool Led::set(Color l_color)
{
	switch (l_color) {
	case Color::OFF:
		digitalWrite(m_green, HIGH);
		digitalWrite(m_red, HIGH);
		digitalWrite(m_blue, HIGH);
		break;
	case Color::GREEEN:
		digitalWrite(m_green, LOW);
		digitalWrite(m_red, HIGH);
		digitalWrite(m_blue, HIGH);
		break;
	case Color::RED:
		digitalWrite(m_green, HIGH);
		digitalWrite(m_red, LOW);
		digitalWrite(m_blue, HIGH);
		break;
	case Color::BLUE:
		digitalWrite(m_green, HIGH);
		digitalWrite(m_red, HIGH);
		digitalWrite(m_blue, LOW);
		break;
	case Color::YELLOW:
		digitalWrite(m_green, LOW);
		digitalWrite(m_red, LOW);
		digitalWrite(m_blue, HIGH);
		break;
	case Color::MAGENTA:
		digitalWrite(m_green, HIGH);
		digitalWrite(m_red, LOW);
		digitalWrite(m_blue, LOW);
		break;
	case Color::CYAN:
		digitalWrite(m_green, LOW);
		digitalWrite(m_red, HIGH);
		digitalWrite(m_blue, LOW);
		break;
	case Color::WHITE:
		digitalWrite(m_green, LOW);
		digitalWrite(m_red, LOW);
		digitalWrite(m_blue, LOW);
		break;
	}

	return true;
}