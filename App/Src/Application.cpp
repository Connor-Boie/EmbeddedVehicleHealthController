#include "Application.hpp"

#include "main.h"

void Application::initialize()
{
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    blinkCount_ = 0U;
}

void Application::run()
{
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    ++blinkCount_;

    HAL_Delay(500U);
}

std::uint32_t Application::blinkCount() const
{
    return blinkCount_;
}
