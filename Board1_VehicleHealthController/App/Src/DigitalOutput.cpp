#include "DigitalOutput.hpp"


DigitalOutput::DigitalOutput(GPIO_TypeDef* port, std::uint16_t pin)
	: port_{port},
	  pin_{pin}
{
}

void DigitalOutput::turnOn()
{
	HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET);
}

void DigitalOutput::turnOff()
{
	HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET);
}

void DigitalOutput::toggle()
{
	HAL_GPIO_TogglePin(port_, pin_);
}

void DigitalOutput::set(bool enabled)
{
	if (enabled)
	{
		turnOn();
	}
	else
	{
		turnOff();
	}
}




