#include "Application.hpp"

#include "main.h"

namespace
{
constexpr std::uint32_t ButtonDebouncePeriodMs = 30U;
}

Application::Application()
    : statusLed_{LD2_GPIO_Port, LD2_Pin},
      buttonDebouncer_{ButtonDebouncePeriodMs}
{
}

void Application::initialize()
{
    statusLed_.turnOff();
    buttonPressCount_ = 0U;

    const std::uint32_t currentTimeMs = HAL_GetTick();
    const bool initialPressed = readUserButtonPressed();

    buttonDebouncer_.initialize(initialPressed, currentTimeMs);
}

void Application::run()
{
    const std::uint32_t currentTimeMs = HAL_GetTick();
    const bool rawPressed = readUserButtonPressed();

    buttonDebouncer_.update(rawPressed, currentTimeMs);

    if (buttonDebouncer_.pressedEvent())
    {
        statusLed_.toggle();
        ++buttonPressCount_;
    }

    HAL_Delay(1U);
}

std::uint32_t Application::buttonPressCount() const
{
    return buttonPressCount_;
}

bool Application::readUserButtonPressed() const
{
    return BSP_PB_GetState(BUTTON_USER) == GPIO_PIN_RESET;
}
