#include "Application.hpp"

#include "main.h"

namespace
{
constexpr std::uint32_t ButtonDebouncePeriodMs = 30U;
constexpr std::uint32_t ButtonSamplePeriodMs = 5U;
constexpr std::uint32_t HeartbeatPeriodMs = 500U;
constexpr std::uint32_t HealthCheckPeriodMs = 1000U;

constexpr std::uint32_t ButtonTaskTimeoutMs = 50U;
}

Application::Application()
    : statusLed_{LD2_GPIO_Port, LD2_Pin},
      buttonDebouncer_{ButtonDebouncePeriodMs},
      buttonSampleTimer_{ButtonSamplePeriodMs},
      heartbeatTimer_{HeartbeatPeriodMs},
      healthCheckTimer_{HealthCheckPeriodMs}
{
}

void Application::initialize()
{
    statusLed_.turnOff();

    buttonPressCount_ = 0U;
    heartbeatExecutionCount_ = 0U;
    healthCheckCount_ = 0U;

    heartbeatEnabled_ = true;
    systemHealthy_ = true;

    const std::uint32_t currentTimeMs = HAL_GetTick();
    const bool initialPressed = readUserButtonPressed();

    lastButtonTaskTimeMs_ = currentTimeMs;

    buttonDebouncer_.initialize(initialPressed, currentTimeMs);

    buttonSampleTimer_.initialize(currentTimeMs);
    heartbeatTimer_.initialize(currentTimeMs);
    healthCheckTimer_.initialize(currentTimeMs);
}

void Application::run()
{
    const std::uint32_t currentTimeMs = HAL_GetTick();

    if (buttonSampleTimer_.isDue(currentTimeMs))
    {
        processButton(currentTimeMs);
    }

    if (heartbeatTimer_.isDue(currentTimeMs))
    {
        updateHeartbeat();
    }

    if (healthCheckTimer_.isDue(currentTimeMs))
    {
        performHealthCheck(currentTimeMs);
    }
}

std::uint32_t Application::buttonPressCount() const
{
    return buttonPressCount_;
}

std::uint32_t Application::heartbeatExecutionCount() const
{
    return heartbeatExecutionCount_;
}

std::uint32_t Application::healthCheckCount() const
{
    return healthCheckCount_;
}

bool Application::heartbeatEnabled() const
{
    return heartbeatEnabled_;
}

bool Application::systemHealthy() const
{
    return systemHealthy_;
}

void Application::processButton(std::uint32_t currentTimeMs)
{
    lastButtonTaskTimeMs_ = currentTimeMs;

    const bool rawPressed = readUserButtonPressed();

    buttonDebouncer_.update(rawPressed, currentTimeMs);

    if (buttonDebouncer_.pressedEvent())
    {
        heartbeatEnabled_ = !heartbeatEnabled_;
        ++buttonPressCount_;

        if (!heartbeatEnabled_)
        {
            statusLed_.turnOff();
        }
    }
}

void Application::updateHeartbeat()
{
    if (!heartbeatEnabled_)
    {
        return;
    }

    statusLed_.toggle();
    ++heartbeatExecutionCount_;
}

void Application::performHealthCheck(std::uint32_t currentTimeMs)
{
    const std::uint32_t timeSinceButtonTaskMs =
        currentTimeMs - lastButtonTaskTimeMs_;

    systemHealthy_ = timeSinceButtonTaskMs <= ButtonTaskTimeoutMs;

    ++healthCheckCount_;
}

bool Application::readUserButtonPressed() const
{
    return BSP_PB_GetState(BUTTON_USER) == GPIO_PIN_RESET;
}
