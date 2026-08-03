#include "Application.hpp"

#include "main.h"

extern "C"
{
extern IWDG_HandleTypeDef hiwdg;
extern UART_HandleTypeDef huart2;
}

namespace
{
constexpr std::uint32_t ButtonDebouncePeriodMs = 30U;
constexpr std::uint32_t ButtonSamplePeriodMs = 5U;
constexpr std::uint32_t HeartbeatPeriodMs = 500U;
constexpr std::uint32_t HealthCheckPeriodMs = 1000U;
constexpr std::uint32_t TelemetryPeriodMs = 1000U;
constexpr std::uint32_t WatchdogRefreshPeriodMs = 500U;

constexpr std::uint32_t ButtonTaskTimeoutMs = 50U;
}

Application::Application()
    : statusLed_{LD2_GPIO_Port, LD2_Pin},
      buttonDebouncer_{ButtonDebouncePeriodMs},
      faultInjector_{},
      faultManager_{},
      resetCauseDetector_{},
      uartReceiver_{},
      telemetry_{&huart2},
      watchdog_{&hiwdg},
      buttonSampleTimer_{ButtonSamplePeriodMs},
      heartbeatTimer_{HeartbeatPeriodMs},
      healthCheckTimer_{HealthCheckPeriodMs},
      telemetryTimer_{TelemetryPeriodMs},
      watchdogRefreshTimer_{WatchdogRefreshPeriodMs}
{
}

void Application::initialize()
{
    resetCauseDetector_.capture();

    statusLed_.turnOff();

    receivedLineCount_ = 0U;
    validCommandCount_ = 0U;
    invalidCommandCount_ = 0U;

    buttonPressCount_ = 0U;
    heartbeatExecutionCount_ = 0U;
    healthCheckCount_ = 0U;

    timerInterruptCount_ = 0U;
    processedTimerEventCount_ = 0U;
    previousHealthCheckTimerCount_ = 0U;

    heartbeatEnabled_ = true;
    systemHealthy_ = true;
    hardwareTimerActive_ = false;
    watchdogRefreshEnabled_ = true;

    const std::uint32_t currentTimeMs = HAL_GetTick();
    const bool initialPressed = readUserButtonPressed();

    lastButtonTaskTimeMs_ = currentTimeMs;

    buttonDebouncer_.initialize(initialPressed, currentTimeMs);

    buttonSampleTimer_.initialize(currentTimeMs);
    heartbeatTimer_.initialize(currentTimeMs);
    healthCheckTimer_.initialize(currentTimeMs);
    telemetryTimer_.initialize(currentTimeMs);
    watchdogRefreshTimer_.initialize(currentTimeMs);

    refreshWatchdog();
}

void Application::run()
{
    const std::uint32_t currentTimeMs = HAL_GetTick();

    processTimerEvents();
    processUartReceive(currentTimeMs);

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

    if (telemetryTimer_.isDue(currentTimeMs))
    {
        sendTelemetry(currentTimeMs);
    }

    if (watchdogRefreshTimer_.isDue(currentTimeMs))
    {
        refreshWatchdog();
    }
}

void Application::onTimerInterrupt()
{
    ++timerInterruptCount_;
}

void Application::onUartByteReceived(std::uint8_t byte)
{
    uartReceiver_.onByteReceivedFromInterrupt(byte);
}

void Application::onUartReceiveError()
{
    uartReceiver_.onReceiveErrorFromInterrupt();
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

std::uint32_t Application::timerInterruptCount() const
{
    return timerInterruptCount_;
}

std::uint32_t Application::processedTimerEventCount() const
{
    return processedTimerEventCount_;
}

std::uint32_t Application::telemetryMessageCount() const
{
    return telemetry_.messageCount();
}

std::uint32_t Application::telemetryFailureCount() const
{
    return telemetry_.failureCount();
}

std::uint32_t Application::receivedLineCount() const
{
    return receivedLineCount_;
}

std::uint32_t Application::validCommandCount() const
{
    return validCommandCount_;
}

std::uint32_t Application::invalidCommandCount() const
{
    return invalidCommandCount_;
}

ResetCause Application::resetCause() const
{
    return resetCauseDetector_.primaryCause();
}

std::uint32_t Application::resetCauseMask() const
{
    return resetCauseDetector_.causeMask();
}

std::uint32_t Application::activeFaultMask() const
{
    return faultManager_.activeFaultMask();
}

std::uint32_t Application::latchedFaultMask() const
{
    return faultManager_.latchedFaultMask();
}

std::uint32_t Application::injectedFaultMask() const
{
    return faultInjector_.injectedFaultMask();
}

std::uint32_t Application::watchdogRefreshCount() const
{
    return watchdog_.refreshCount();
}

std::uint32_t Application::watchdogFailureCount() const
{
    return watchdog_.failureCount();
}

bool Application::heartbeatEnabled() const
{
    return heartbeatEnabled_;
}

bool Application::systemHealthy() const
{
    return systemHealthy_;
}

bool Application::hardwareTimerActive() const
{
    return hardwareTimerActive_;
}

bool Application::watchdogRefreshEnabled() const
{
    return watchdogRefreshEnabled_;
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

void Application::processUartReceive(
    std::uint32_t currentTimeMs)
{
    uartReceiver_.process();

    while (uartReceiver_.readLine(
        receivedLine_,
        sizeof(receivedLine_)))
    {
        ++receivedLineCount_;

        const CommandType command =
            CommandParser::parse(receivedLine_);

        handleCommand(command, currentTimeMs);
    }
}

void Application::handleCommand(
    CommandType command,
    std::uint32_t currentTimeMs)
{
    switch (command)
    {
        case CommandType::Status:
        {
            ++validCommandCount_;
            sendTelemetry(currentTimeMs);
            break;
        }

        case CommandType::Faults:
        {
            ++validCommandCount_;
            sendTelemetry(currentTimeMs);
            break;
        }

        case CommandType::ResetCause:
        {
            ++validCommandCount_;
            sendTelemetry(currentTimeMs);
            break;
        }

        case CommandType::HeartbeatOn:
        {
            ++validCommandCount_;
            heartbeatEnabled_ = true;

            const bool sent =
                telemetry_.sendText("OK HEARTBEAT ON");

            static_cast<void>(sent);
            break;
        }

        case CommandType::HeartbeatOff:
        {
            ++validCommandCount_;
            heartbeatEnabled_ = false;
            statusLed_.turnOff();

            const bool sent =
                telemetry_.sendText("OK HEARTBEAT OFF");

            static_cast<void>(sent);
            break;
        }

        case CommandType::InjectButtonFault:
        {
            ++validCommandCount_;

            faultInjector_.injectFault(
                Fault::ButtonTaskTimeout);

            const bool sent =
                telemetry_.sendText(
                    "OK BUTTON FAULT INJECTED");

            static_cast<void>(sent);
            break;
        }

        case CommandType::InjectTimerFault:
        {
            ++validCommandCount_;

            faultInjector_.injectFault(
                Fault::HardwareTimerInactive);

            const bool sent =
                telemetry_.sendText(
                    "OK TIMER FAULT INJECTED");

            static_cast<void>(sent);
            break;
        }

        case CommandType::WatchdogTest:
        {
            ++validCommandCount_;

            const bool sent =
                telemetry_.sendText(
                    "OK WATCHDOG RESET EXPECTED");

            static_cast<void>(sent);

            watchdogRefreshEnabled_ = false;
            break;
        }

        case CommandType::ClearCounters:
        {
            clearApplicationCounters();

            const bool sent =
                telemetry_.sendText("OK COUNTERS CLEARED");

            static_cast<void>(sent);
            break;
        }

        case CommandType::ClearFaults:
        {
            ++validCommandCount_;
            faultManager_.clearLatchedFaults();

            const bool sent =
                telemetry_.sendText("OK FAULTS CLEARED");

            static_cast<void>(sent);
            break;
        }

        case CommandType::ClearInjectedFaults:
        {
            ++validCommandCount_;
            faultInjector_.clearAll();

            const bool sent =
                telemetry_.sendText(
                    "OK INJECTED FAULTS CLEARED");

            static_cast<void>(sent);
            break;
        }

        case CommandType::Invalid:
        default:
        {
            ++invalidCommandCount_;

            const bool sent =
                telemetry_.sendText(
                    "ERROR INVALID COMMAND");

            static_cast<void>(sent);
            break;
        }
    }
}

void Application::clearApplicationCounters()
{
    receivedLineCount_ = 0U;
    validCommandCount_ = 0U;
    invalidCommandCount_ = 0U;

    buttonPressCount_ = 0U;
    heartbeatExecutionCount_ = 0U;
    healthCheckCount_ = 0U;
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

    const bool buttonTaskHealthy =
        timeSinceButtonTaskMs <= ButtonTaskTimeoutMs;

    const std::uint32_t currentTimerInterruptCount =
        timerInterruptCount_;

    hardwareTimerActive_ =
        currentTimerInterruptCount !=
        previousHealthCheckTimerCount_;

    previousHealthCheckTimerCount_ =
        currentTimerInterruptCount;

    const bool buttonTaskFaultActive =
        (!buttonTaskHealthy) ||
        faultInjector_.isInjected(
            Fault::ButtonTaskTimeout);

    const bool hardwareTimerFaultActive =
        (!hardwareTimerActive_) ||
        faultInjector_.isInjected(
            Fault::HardwareTimerInactive);

    faultManager_.setFault(
        Fault::ButtonTaskTimeout,
        buttonTaskFaultActive);

    faultManager_.setFault(
        Fault::HardwareTimerInactive,
        hardwareTimerFaultActive);

    systemHealthy_ = !faultManager_.hasActiveFaults();

    ++healthCheckCount_;
}

void Application::processTimerEvents()
{
    const std::uint32_t observedInterruptCount =
        timerInterruptCount_;

    if (observedInterruptCount ==
        processedTimerEventCount_)
    {
        return;
    }

    processedTimerEventCount_ =
        observedInterruptCount;
}

void Application::refreshWatchdog()
{
    if (!watchdogRefreshEnabled_)
    {
        return;
    }

    const bool refreshed = watchdog_.refresh();

    static_cast<void>(refreshed);
}

void Application::sendTelemetry(std::uint32_t currentTimeMs)
{
    const bool sent = telemetry_.sendStatus(
        currentTimeMs,
        resetCauseDetector_.primaryCauseName(),
        resetCauseDetector_.causeMask(),
        buttonPressCount_,
        heartbeatEnabled_,
        systemHealthy_,
        hardwareTimerActive_,
        timerInterruptCount_,
        receivedLineCount_,
        validCommandCount_,
        invalidCommandCount_,
        faultManager_.activeFaultMask(),
        faultManager_.latchedFaultMask(),
        faultInjector_.injectedFaultMask(),
        watchdogRefreshEnabled_,
        watchdog_.refreshCount(),
        watchdog_.failureCount(),
        uartReceiver_.droppedByteCount(),
        uartReceiver_.overflowLineCount(),
        uartReceiver_.receiveErrorCount());

    static_cast<void>(sent);
}

bool Application::readUserButtonPressed() const
{
    return BSP_PB_GetState(BUTTON_USER) == GPIO_PIN_RESET;
}
