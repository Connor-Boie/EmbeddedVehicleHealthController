#include "Application.hpp"

#include "main.h"

namespace
{
constexpr std::uint32_t ButtonDebouncePeriodMs = 30U;
constexpr std::uint32_t ButtonSamplePeriodMs = 5U;
}

Application::Application()
    : statusLed_{LD2_GPIO_Port, LD2_Pin},
      buttonDebouncer_{ButtonDebouncePeriodMs},
      buttonSampleTimer_{ButtonSamplePeriodMs}
{
}

void Application::initialize()
{
    statusLed_.turnOff();
    buttonPressCount_ = 0U;

    const std::uint32_t currentTimeMs = HAL_GetTick();
    const bool initialPressed = readUserButtonPressed();

    buttonDebouncer_.initialize(initialPressed, currentTimeMs);
    buttonSampleTimer_.initialize(currentTimeMs);
}

void Application::run()
{
    const std::uint32_t currentTimeMs = HAL_GetTick();

    if (buttonSampleTimer_.isDue(currentTimeMs))
    {
        processButton(currentTimeMs);
    }
}

std::uint32_t Application::buttonPressCount() const
{
    return buttonPressCount_;
}

void Application::processButton(std::uint32_t currentTimeMs)
{
    const bool rawPressed = readUserButtonPressed();

    buttonDebouncer_.update(rawPressed, currentTimeMs);

    if (buttonDebouncer_.pressedEvent())
    {
        statusLed_.toggle();
        ++buttonPressCount_;
    }
}

bool Application::readUserButtonPressed() const
{
    return BSP_PB_GetState(BUTTON_USER) == GPIO_PIN_RESET;
}
