#include "Application.hpp"

#include "main.h"

Application::Application()
	: statusLed_{LD2_GPIO_Port, LD2_Pin}
{
}

void Application::initialize()
{
    statusLed_.turnOff();
    blinkCount_ = 0U;
}

void Application::run()
{
    statusLed_.toggle();
    ++blinkCount_;

    HAL_Delay(500U);
}

std::uint32_t Application::blinkCount() const
{
    return blinkCount_;
}
