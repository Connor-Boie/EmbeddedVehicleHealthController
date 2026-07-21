#ifndef DIGITAL_OUTPUT_HPP
#define DIGITAL_OUTPUT_HPP

#include "stm32f4xx_hal.h"

#include <cstdint>

class DigitalOutput
{
public:
	DigitalOutput(GPIO_TypeDef* port, std::uint16_t pin);

	void turnOn();
	void turnOff();
	void toggle();
	void set(bool enabled);

private:
	GPIO_TypeDef* port_;
	std::uint16_t pin_;
};

#endif
