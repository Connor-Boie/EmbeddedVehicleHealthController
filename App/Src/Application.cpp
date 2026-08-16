#include "Application.hpp"

#include "main.h"

extern "C"
{
extern I2C_HandleTypeDef hi2c1;
extern IWDG_HandleTypeDef hiwdg;
extern SPI_HandleTypeDef hspi2;
extern UART_HandleTypeDef huart2;
}

namespace
{
constexpr std::uint8_t
    TemperatureSensorAAddress = 0x18U;

constexpr std::uint8_t
    TemperatureSensorBAddress = 0x19U;

constexpr std::uint32_t
    ButtonDebouncePeriodMs = 30U;

constexpr std::uint32_t
    ButtonSamplePeriodMs = 5U;

constexpr std::uint32_t
    HeartbeatPeriodMs = 500U;

constexpr std::uint32_t
    HealthCheckPeriodMs = 1000U;

constexpr std::uint32_t
    TemperatureSamplePeriodMs = 1000U;

constexpr std::uint32_t
    TelemetryPeriodMs = 1000U;

constexpr std::uint32_t
    WatchdogRefreshPeriodMs = 500U;

constexpr std::uint32_t
    ButtonTaskTimeoutMs = 50U;

constexpr std::uint32_t
    FlashTestSectorAddress =
        W25q64::CapacityBytes -
        W25q64::SectorSizeBytes;

static_assert(
    DiagnosticLogger::RegionEndAddress <=
        FlashTestSectorAddress,
    "Diagnostic log must not overlap FLASH TEST sector");
}

Application::Application()
    : statusLed_{
          LD2_GPIO_Port,
          LD2_Pin},

      buttonDebouncer_{
          ButtonDebouncePeriodMs},

      faultInjector_{},
      faultManager_{},

      resetCauseDetector_{},

      temperatureSensorA_{
          &hi2c1,
          TemperatureSensorAAddress},

      temperatureSensorB_{
          &hi2c1,
          TemperatureSensorBAddress},

      temperatureHealthMonitor_{},

      flash_{
          &hspi2,
          FLASH_CS_GPIO_Port,
          FLASH_CS_Pin},

      diagnosticLogger_{
          &flash_},

      uartReceiver_{},

      telemetry_{
          &huart2},

      watchdog_{
          &hiwdg},

      buttonSampleTimer_{
          ButtonSamplePeriodMs},

      heartbeatTimer_{
          HeartbeatPeriodMs},

      healthCheckTimer_{
          HealthCheckPeriodMs},

      temperatureSampleTimer_{
          TemperatureSamplePeriodMs},

      telemetryTimer_{
          TelemetryPeriodMs},

      watchdogRefreshTimer_{
          WatchdogRefreshPeriodMs}
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

    systemHealthy_ = true;
    hardwareTimerActive_ = false;
    watchdogRefreshEnabled_ = true;

    previousLoggedFaultMask_ = 0U;

    flashTestRun_ = false;
    flashTestPassed_ = false;

    const std::uint32_t currentTimeMs =
        HAL_GetTick();

    const bool initialPressed =
        readUserButtonPressed();

    lastButtonTaskTimeMs_ =
        currentTimeMs;

    buttonDebouncer_.initialize(
        initialPressed,
        currentTimeMs);

    buttonSampleTimer_.initialize(
        currentTimeMs);

    heartbeatTimer_.initialize(
        currentTimeMs);

    healthCheckTimer_.initialize(
        currentTimeMs);

    temperatureSampleTimer_.initialize(
        currentTimeMs);

    telemetryTimer_.initialize(
        currentTimeMs);

    watchdogRefreshTimer_.initialize(
        currentTimeMs);

    const bool sensorAInitialized =
        temperatureSensorA_.initialize();

    const bool sensorBInitialized =
        temperatureSensorB_.initialize();

    if (sensorAInitialized)
    {
        const bool sensorARead =
            temperatureSensorA_
                .readTemperature();

        static_cast<void>(
            sensorARead);
    }

    if (sensorBInitialized)
    {
        const bool sensorBRead =
            temperatureSensorB_
                .readTemperature();

        static_cast<void>(
            sensorBRead);
    }

    temperatureHealthMonitor_.update(
        temperatureSensorA_.available(),
        temperatureSensorA_
            .temperatureMilliCelsius(),
        temperatureSensorB_.available(),
        temperatureSensorB_
            .temperatureMilliCelsius());

    const bool flashInitialized =
        flash_.initialize();

    if (flashInitialized)
    {
        refreshWatchdog();

        const bool loggerInitialized =
            diagnosticLogger_.initialize();

        if (loggerInitialized)
        {
            const bool startupLogged =
                diagnosticLogger_.append(
                    DiagnosticEventType::
                        SystemStartup,
                    HAL_GetTick(),
                    static_cast<std::uint32_t>(
                        resetCauseDetector_
                            .primaryCause()),
                    resetCauseDetector_
                        .causeMask());

            static_cast<void>(
                startupLogged);
        }
    }

    refreshWatchdog();
}

void Application::run()
{
    const std::uint32_t currentTimeMs =
        HAL_GetTick();

    processTimerEvents();
    processUartReceive(currentTimeMs);

    if (buttonSampleTimer_.isDue(
        currentTimeMs))
    {
        processButton(
            currentTimeMs);
    }

    if (heartbeatTimer_.isDue(
        currentTimeMs))
    {
        updateHeartbeat();
    }

    if (temperatureSampleTimer_.isDue(
        currentTimeMs))
    {
        processTemperatures();
    }

    if (healthCheckTimer_.isDue(
        currentTimeMs))
    {
        performHealthCheck(
            currentTimeMs);
    }

    if (telemetryTimer_.isDue(
        currentTimeMs))
    {
        sendTelemetry(
            currentTimeMs);
    }

    if (watchdogRefreshTimer_.isDue(
        currentTimeMs))
    {
        refreshWatchdog();
    }
}

void Application::onTimerInterrupt()
{
    ++timerInterruptCount_;
}

void Application::onUartByteReceived(
    std::uint8_t byte)
{
    uartReceiver_
        .onByteReceivedFromInterrupt(
            byte);
}

void Application::onUartReceiveError()
{
    uartReceiver_
        .onReceiveErrorFromInterrupt();
}

std::uint32_t
Application::buttonPressCount() const
{
    return buttonPressCount_;
}

std::uint32_t
Application::heartbeatExecutionCount() const
{
    return heartbeatExecutionCount_;
}

std::uint32_t
Application::healthCheckCount() const
{
    return healthCheckCount_;
}

std::uint32_t
Application::timerInterruptCount() const
{
    return timerInterruptCount_;
}

std::uint32_t
Application::processedTimerEventCount() const
{
    return processedTimerEventCount_;
}

std::uint32_t
Application::telemetryMessageCount() const
{
    return telemetry_.messageCount();
}

std::uint32_t
Application::telemetryFailureCount() const
{
    return telemetry_.failureCount();
}

std::uint32_t
Application::receivedLineCount() const
{
    return receivedLineCount_;
}

std::uint32_t
Application::validCommandCount() const
{
    return validCommandCount_;
}

std::uint32_t
Application::invalidCommandCount() const
{
    return invalidCommandCount_;
}

ResetCause
Application::resetCause() const
{
    return resetCauseDetector_
        .primaryCause();
}

std::uint32_t
Application::resetCauseMask() const
{
    return resetCauseDetector_
        .causeMask();
}

std::int32_t
Application::sensorATemperatureMilliCelsius()
    const
{
    return temperatureSensorA_
        .temperatureMilliCelsius();
}

std::int32_t
Application::sensorBTemperatureMilliCelsius()
    const
{
    return temperatureSensorB_
        .temperatureMilliCelsius();
}

bool Application::sensorAAvailable() const
{
    return temperatureSensorA_
        .available();
}

bool Application::sensorBAvailable() const
{
    return temperatureSensorB_
        .available();
}

TemperatureMode
Application::temperatureMode() const
{
    return temperatureHealthMonitor_
        .mode();
}

bool
Application::selectedTemperatureValid() const
{
    return temperatureHealthMonitor_
        .selectedTemperatureValid();
}

std::int32_t
Application::selectedTemperatureMilliCelsius()
    const
{
    return temperatureHealthMonitor_
        .selectedTemperatureMilliCelsius();
}

bool Application::flashAvailable() const
{
    return flash_.available();
}

std::uint32_t
Application::flashJedecId() const
{
    return flash_.jedecId();
}

std::uint32_t
Application::activeFaultMask() const
{
    return faultManager_
        .activeFaultMask();
}

std::uint32_t
Application::latchedFaultMask() const
{
    return faultManager_
        .latchedFaultMask();
}

std::uint32_t
Application::injectedFaultMask() const
{
    return faultInjector_
        .injectedFaultMask();
}

std::uint32_t
Application::watchdogRefreshCount() const
{
    return watchdog_.refreshCount();
}

std::uint32_t
Application::watchdogFailureCount() const
{
    return watchdog_.failureCount();
}

bool Application::systemHealthy() const
{
    return systemHealthy_;
}

bool
Application::hardwareTimerActive() const
{
    return hardwareTimerActive_;
}

bool
Application::watchdogRefreshEnabled() const
{
    return watchdogRefreshEnabled_;
}

void Application::processButton(
    std::uint32_t currentTimeMs)
{
    lastButtonTaskTimeMs_ =
        currentTimeMs;

    const bool rawPressed =
        readUserButtonPressed();

    buttonDebouncer_.update(
        rawPressed,
        currentTimeMs);

    if (buttonDebouncer_.pressedEvent())
    {
        ++buttonPressCount_;

        sendTelemetry(
            currentTimeMs);
    }
}

void Application::processTemperatures()
{
    const bool sensorARead =
        temperatureSensorA_
            .readTemperature();

    const bool sensorBRead =
        temperatureSensorB_
            .readTemperature();

    static_cast<void>(
        sensorARead);

    static_cast<void>(
        sensorBRead);

    temperatureHealthMonitor_.update(
        temperatureSensorA_.available(),
        temperatureSensorA_
            .temperatureMilliCelsius(),
        temperatureSensorB_.available(),
        temperatureSensorB_
            .temperatureMilliCelsius());
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
            CommandParser::parse(
                receivedLine_);

        handleCommand(
            command,
            currentTimeMs);
    }
}

void Application::handleCommand(
    CommandType command,
    std::uint32_t currentTimeMs)
{
    switch (command)
    {
        case CommandType::Status:
        case CommandType::Faults:
        case CommandType::ResetCause:
        case CommandType::Temperatures:
        case CommandType::FlashStatus:
        {
            ++validCommandCount_;

            sendTelemetry(
                currentTimeMs);

            break;
        }

        case CommandType::FlashTest:
        {
            ++validCommandCount_;

            runFlashSelfTest();

            sendTelemetry(
                HAL_GetTick());

            break;
        }

        case CommandType::LogErase:
        {
            ++validCommandCount_;

            refreshWatchdog();

            const bool erased =
                diagnosticLogger_.eraseAll();

            refreshWatchdog();

            const bool sent =
                telemetry_.sendText(
                    erased
                        ? "OK DIAGNOSTIC LOG ERASED"
                        : "ERROR DIAGNOSTIC LOG ERASE FAILED");

            static_cast<void>(
                sent);

            sendTelemetry(
                HAL_GetTick());

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

            static_cast<void>(
                sent);

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

            static_cast<void>(
                sent);

            break;
        }

        case CommandType::WatchdogTest:
        {
            ++validCommandCount_;

            const bool sent =
                telemetry_.sendText(
                    "OK WATCHDOG RESET EXPECTED");

            static_cast<void>(
                sent);

            watchdogRefreshEnabled_ =
                false;

            break;
        }

        case CommandType::ClearCounters:
        {
            clearApplicationCounters();

            const bool sent =
                telemetry_.sendText(
                    "OK COUNTERS CLEARED");

            static_cast<void>(
                sent);

            break;
        }

        case CommandType::ClearFaults:
        {
            ++validCommandCount_;

            faultManager_
                .clearLatchedFaults();

            const bool sent =
                telemetry_.sendText(
                    "OK FAULTS CLEARED");

            static_cast<void>(
                sent);

            break;
        }

        case CommandType::ClearInjectedFaults:
        {
            ++validCommandCount_;

            faultInjector_.clearAll();

            const bool sent =
                telemetry_.sendText(
                    "OK INJECTED FAULTS CLEARED");

            static_cast<void>(
                sent);

            break;
        }

        case CommandType::Invalid:
        default:
        {
            ++invalidCommandCount_;

            const bool sent =
                telemetry_.sendText(
                    "ERROR INVALID COMMAND");

            static_cast<void>(
                sent);

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
    statusLed_.toggle();

    ++heartbeatExecutionCount_;
}

void Application::performHealthCheck(
    std::uint32_t currentTimeMs)
{
    const std::uint32_t
        previousActiveFaultMask =
            previousLoggedFaultMask_;

    const std::uint32_t
        timeSinceButtonTaskMs =
            currentTimeMs -
            lastButtonTaskTimeMs_;

    const bool buttonTaskHealthy =
        timeSinceButtonTaskMs <=
        ButtonTaskTimeoutMs;

    const std::uint32_t
        currentTimerInterruptCount =
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

    faultManager_.setFault(
        Fault::TemperatureSensorAUnavailable,
        temperatureHealthMonitor_
            .sensorAFaultActive());

    faultManager_.setFault(
        Fault::TemperatureSensorBUnavailable,
        temperatureHealthMonitor_
            .sensorBFaultActive());

    faultManager_.setFault(
        Fault::TemperatureDisagreement,
        temperatureHealthMonitor_
            .disagreementFaultActive());

    faultManager_.setFault(
        Fault::Overtemperature,
        temperatureHealthMonitor_
            .overtemperatureFaultActive());

    const std::uint32_t
        currentActiveFaultMask =
            faultManager_.activeFaultMask();

    const std::uint32_t
        newlyActivatedFaults =
            currentActiveFaultMask &
            ~previousActiveFaultMask;

    const std::uint32_t
        newlyClearedFaults =
            previousActiveFaultMask &
            ~currentActiveFaultMask;

    if (newlyActivatedFaults != 0U)
    {
        const bool logged =
            diagnosticLogger_.append(
                DiagnosticEventType::
                    FaultActivated,
                currentTimeMs,
                newlyActivatedFaults,
                currentActiveFaultMask);

        static_cast<void>(
            logged);
    }

    if (newlyClearedFaults != 0U)
    {
        const bool logged =
            diagnosticLogger_.append(
                DiagnosticEventType::
                    FaultCleared,
                currentTimeMs,
                newlyClearedFaults,
                currentActiveFaultMask);

        static_cast<void>(
            logged);
    }

    previousLoggedFaultMask_ =
        currentActiveFaultMask;

    systemHealthy_ =
        !faultManager_.hasActiveFaults();

    ++healthCheckCount_;
}

void Application::processTimerEvents()
{
    const std::uint32_t
        observedInterruptCount =
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

    const bool refreshed =
        watchdog_.refresh();

    static_cast<void>(
        refreshed);
}

void Application::sendTelemetry(
    std::uint32_t currentTimeMs)
{
    const bool sent =
        telemetry_.sendStatus(
            currentTimeMs,

            resetCauseDetector_
                .primaryCauseName(),

            resetCauseDetector_
                .causeMask(),

            temperatureSensorA_
                .available(),

            temperatureSensorA_
                .temperatureMilliCelsius(),

            temperatureSensorA_
                .successfulReadCount(),

            temperatureSensorA_
                .failureCount(),

            temperatureSensorB_
                .available(),

            temperatureSensorB_
                .temperatureMilliCelsius(),

            temperatureSensorB_
                .successfulReadCount(),

            temperatureSensorB_
                .failureCount(),

            temperatureHealthMonitor_
                .modeName(),

            temperatureHealthMonitor_
                .selectedTemperatureValid(),

            temperatureHealthMonitor_
                .selectedTemperatureMilliCelsius(),

            temperatureHealthMonitor_
                .disagreementMilliCelsius(),

            flash_.available(),
            flash_.jedecId(),
            flash_.failureCount(),
            flashTestRun_,
            flashTestPassed_,

            diagnosticLogger_
                .initialized(),

            diagnosticLogger_
                .recordCount(),

            DiagnosticLogger::
                RecordCapacity,

            diagnosticLogger_
                .full(),

            diagnosticLogger_
                .invalidRecordCount(),

            diagnosticLogger_
                .failureCount(),

            diagnosticLogger_
                .nextSequence(),

            diagnosticLogger_
                .lastRecordValid(),

            diagnosticLogger_
                .lastRecord().eventType,

            diagnosticLogger_
                .lastRecord().sequence,

            diagnosticLogger_
                .lastRecord().uptimeMs,

            diagnosticLogger_
                .lastRecord().data0,

            diagnosticLogger_
                .lastRecord().data1,

            buttonPressCount_,
            heartbeatExecutionCount_,

            systemHealthy_,
            hardwareTimerActive_,
            timerInterruptCount_,

            receivedLineCount_,
            validCommandCount_,
            invalidCommandCount_,

            faultManager_
                .activeFaultMask(),

            faultManager_
                .latchedFaultMask(),

            faultInjector_
                .injectedFaultMask(),

            watchdogRefreshEnabled_,

            watchdog_
                .refreshCount(),

            watchdog_
                .failureCount(),

            uartReceiver_
                .droppedByteCount(),

            uartReceiver_
                .overflowLineCount(),

            uartReceiver_
                .receiveErrorCount());

    static_cast<void>(
        sent);
}

void Application::runFlashSelfTest()
{
    flashTestRun_ = true;
    flashTestPassed_ = false;

    if (!flash_.available())
    {
        const bool initialized =
            flash_.initialize();

        if (!initialized)
        {
            const bool sent =
                telemetry_.sendText(
                    "ERROR FLASH NOT AVAILABLE");

            static_cast<void>(
                sent);

            return;
        }
    }

    constexpr std::uint8_t testPattern[]{
        0x45U, 0x56U, 0x48U, 0x43U,
        0x2DU, 0x46U, 0x4CU, 0x41U,
        0x53U, 0x48U, 0x2DU, 0x54U,
        0x45U, 0x53U, 0x54U, 0x2DU,
        0x01U, 0x23U, 0x45U, 0x67U,
        0x89U, 0xABU, 0xCDU, 0xEFU,
        0x10U, 0x32U, 0x54U, 0x76U,
        0x98U, 0xBAU, 0xDCU, 0xFEU
    };

    std::uint8_t readback[
        sizeof(testPattern)]{};

    refreshWatchdog();

    if (!flash_.eraseSector(
        FlashTestSectorAddress))
    {
        const bool sent =
            telemetry_.sendText(
                "ERROR FLASH ERASE FAILED");

        static_cast<void>(
            sent);

        return;
    }

    refreshWatchdog();

    if (!flash_.program(
        FlashTestSectorAddress,
        testPattern,
        sizeof(testPattern)))
    {
        const bool sent =
            telemetry_.sendText(
                "ERROR FLASH PROGRAM FAILED");

        static_cast<void>(
            sent);

        return;
    }

    refreshWatchdog();

    if (!flash_.read(
        FlashTestSectorAddress,
        readback,
        sizeof(readback)))
    {
        const bool sent =
            telemetry_.sendText(
                "ERROR FLASH READ FAILED");

        static_cast<void>(
            sent);

        return;
    }

    for (std::size_t index = 0U;
         index < sizeof(testPattern);
         ++index)
    {
        if (readback[index] !=
            testPattern[index])
        {
            const bool sent =
                telemetry_.sendText(
                    "ERROR FLASH VERIFY FAILED");

            static_cast<void>(
                sent);

            return;
        }
    }

    flashTestPassed_ = true;

    const bool sent =
        telemetry_.sendText(
            "OK FLASH TEST PASSED");

    static_cast<void>(
        sent);
}

bool Application::readUserButtonPressed() const
{
    return BSP_PB_GetState(
        BUTTON_USER) ==
        GPIO_PIN_RESET;
}

