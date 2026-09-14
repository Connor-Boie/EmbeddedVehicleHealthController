#include "Application.hpp"

#include "CanProtocol.hpp"
#include "main.h"

#include <cstdio>

extern "C"
{
extern CAN_HandleTypeDef hcan1;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern IWDG_HandleTypeDef hiwdg;
extern UART_HandleTypeDef huart2;
}

namespace
{

constexpr std::uint32_t
    UartTimeoutMs = 100U;

constexpr std::uint32_t
    RgbSelfTestColorTimeMs = 250U;

constexpr std::uint32_t
    FanSelfTestDutyTimeMs = 1000U;

constexpr std::uint32_t
    FanSelfTestOffTimeMs = 500U;

constexpr std::uint32_t
    UserButtonDebounceTimeMs = 30U;

constexpr std::uint32_t
    UserButtonStartupArmTimeMs = 250U;

constexpr std::uint32_t
    StartupGracePeriodMs = 1000U;

constexpr std::uint32_t
    WatchdogRefreshPeriodMs = 500U;

constexpr bool
    WatchdogResetTestEnabled = false;

constexpr std::uint32_t
    ActuatorSelfTestStageTimeMs = 3000U;

constexpr std::uint8_t
    ActuatorSelfTestStageCount = 5U;

CanFrame buildTestVehicleHealthFrame(
    bool temperatureValid,
    std::int16_t temperatureDeciCelsius)
{
    CanFrame frame{};

    frame.id =
        CanProtocol::MessageId::
            VehicleHealthStatus;

    frame.length =
        CanProtocol::
            VehicleHealthStatus::
                PayloadLength;

    frame.data[
        CanProtocol::
            VehicleHealthStatus::
                ProtocolVersionIndex] =
        CanProtocol::ProtocolVersion;

    std::uint8_t statusFlags =
        CanProtocol::
            VehicleHealthStatus::
                SystemHealthyFlag |
        CanProtocol::
            VehicleHealthStatus::
                SensorAAvailableFlag |
        CanProtocol::
            VehicleHealthStatus::
                SensorBAvailableFlag;

    if (temperatureValid)
    {
        statusFlags |=
            CanProtocol::
                VehicleHealthStatus::
                    SelectedTemperatureValidFlag;
    }

    frame.data[
        CanProtocol::
            VehicleHealthStatus::
                StatusFlagsIndex] =
        statusFlags;

    const std::uint16_t temperatureUnsigned =
        static_cast<std::uint16_t>(
            temperatureDeciCelsius);

    frame.data[
        CanProtocol::
            VehicleHealthStatus::
                TemperatureLowByteIndex] =
        static_cast<std::uint8_t>(
            temperatureUnsigned &
            0xFFU);

    frame.data[
        CanProtocol::
            VehicleHealthStatus::
                TemperatureHighByteIndex] =
        static_cast<std::uint8_t>(
            (temperatureUnsigned >>
             8U) &
            0xFFU);

    frame.data[
        CanProtocol::
            VehicleHealthStatus::
                FaultMaskByte0Index] =
        0U;

    frame.data[
        CanProtocol::
            VehicleHealthStatus::
                FaultMaskByte1Index] =
        0U;

    frame.data[
        CanProtocol::
            VehicleHealthStatus::
                FaultMaskByte2Index] =
        0U;

    frame.data[
        CanProtocol::
            VehicleHealthStatus::
                FaultMaskByte3Index] =
        0U;

    return frame;
}

const char* warningColorName(
    WarningColor color)
{
    switch (color)
    {
        case WarningColor::Green:
            return "GREEN";

        case WarningColor::Yellow:
            return "YELLOW";

        case WarningColor::Blue:
            return "BLUE";

        case WarningColor::Orange:
            return "ORANGE";

        case WarningColor::Red:
            return "RED";

        case WarningColor::Magenta:
            return "MAGENTA";
    }

    return "UNKNOWN";
}

const char* buzzerPatternName(
    BuzzerPattern pattern)
{
    switch (pattern)
    {
        case BuzzerPattern::Off:
            return "OFF";

        case BuzzerPattern::SlowBeep:
            return "SLOW_BEEP";

        case BuzzerPattern::FastBeep:
            return "FAST_BEEP";

        case BuzzerPattern::Fault:
            return "FAULT";
    }

    return "UNKNOWN";
}

bool actuatorCommandMatches(
    const ActuatorCommand& command,
    std::uint8_t expectedCoolingDutyPercent,
    WarningColor expectedColor,
    BuzzerPattern expectedBuzzerPattern)
{
    return
        (command.coolingDutyPercent ==
         expectedCoolingDutyPercent) &&
        (command.warningColor ==
         expectedColor) &&
        (command.buzzerPattern ==
         expectedBuzzerPattern);
}

bool rgbIntensityMatches(
    const RgbIntensityPercent& intensity,
    std::uint8_t expectedRed,
    std::uint8_t expectedGreen,
    std::uint8_t expectedBlue)
{
    return
        (intensity.red ==
         expectedRed) &&
        (intensity.green ==
         expectedGreen) &&
        (intensity.blue ==
         expectedBlue);
}

}

Application::Application()
    : canBus_{&hcan1},
      rgbLed_{
          &htim3,
          TIM_CHANNEL_1,
          TIM_CHANNEL_2,
          TIM_CHANNEL_3},
      fanPwm_{
          &htim3,
          TIM_CHANNEL_4},
      buzzerPwm_{
          &htim4,
          TIM_CHANNEL_1},
      watchdog_{&hiwdg}
{
}

void Application::initialize()
{
    remoteVehicleStatus_.reset();

    thermalControlStateMachine_.reset();

    actuatorCommandPolicy_.reset();

    const std::uint32_t currentTimeMs =
        HAL_GetTick();

    buzzerPatternSequencer_.reset(
        currentTimeMs);

    communicationStateInitialized_ =
        false;

    thermalControlStateInitialized_ =
        false;

    startupGraceActive_ =
        true;

    startupTimeMs_ =
        currentTimeMs;

    watchdogLastRefreshTimeMs_ =
        currentTimeMs;

    if (watchdog_.refresh())
    {
        transmitText(
            "BOARD2 READY WATCHDOG ACTIVE\r\n");
    }
    else
    {
        transmitText(
            "BOARD2 ERROR WATCHDOG REFRESH FAILED\r\n");
    }

    if (canBus_.initialize())
    {
        transmitText(
            "BOARD2 READY CAN INITIALIZED\r\n");
    }
    else
    {
        transmitText(
            "BOARD2 ERROR CAN NOT INITIALIZED\r\n");
    }

    if (rgbLed_.initialize())
    {
        transmitText(
            "BOARD2 READY RGB PWM INITIALIZED\r\n");
    }
    else
    {
        transmitText(
            "BOARD2 ERROR RGB PWM NOT INITIALIZED\r\n");
    }

    if (fanPwm_.initialize())
    {
        transmitText(
            "BOARD2 READY FAN PWM INITIALIZED\r\n");
    }
    else
    {
        transmitText(
            "BOARD2 ERROR FAN PWM NOT INITIALIZED\r\n");
    }

    if (buzzerPwm_.initialize())
    {
        transmitText(
            "BOARD2 READY BUZZER PWM INITIALIZED\r\n");
    }
    else
    {
        transmitText(
            "BOARD2 ERROR BUZZER PWM NOT INITIALIZED\r\n");
    }

    runRemoteStatusSelfTest();

    runThermalControlSelfTest();

    runActuatorCommandSelfTest();

    runBuzzerTimingSelfTest();

    runRgbMappingSelfTest();

    reportCommunicationState(
        remoteVehicleStatus_.
            communicationState());

    previousCommunicationState_ =
        remoteVehicleStatus_.
            communicationState();

    communicationStateInitialized_ =
        true;

    thermalControlStateMachine_.update(
        remoteVehicleStatus_);

    const ThermalControlState
        initialThermalState =
            thermalControlStateMachine_.
                state();

    reportThermalControlState(
        initialThermalState);

    previousThermalControlState_ =
        initialThermalState;

    thermalControlStateInitialized_ =
        true;

    actuatorCommandPolicy_.update(
        initialThermalState);

    reportActuatorCommand();

    activeOutputCommand_.coolingDutyPercent =
        0U;

    activeOutputCommand_.warningColor =
        WarningColor::Green;

    activeOutputCommand_.buzzerPattern =
        BuzzerPattern::Off;

    rgbLed_.setColor(
        activeOutputCommand_.
            warningColor);

    fanPwm_.setDutyPercent(
        activeOutputCommand_.
            coolingDutyPercent);

    buzzerPwm_.setEnabled(
        false);

    reportFanPwmState();

    userButtonRawPressed_ =
        readUserButtonPressed();

    userButtonDebouncedPressed_ =
        userButtonRawPressed_;

    userButtonArmed_ =
        false;

    userButtonRawChangeTimeMs_ =
        HAL_GetTick();

    userButtonReleaseStartTimeMs_ =
        HAL_GetTick();

    actuatorSelfTestActive_ =
        false;

    actuatorSelfTestStage_ =
        0U;

    actuatorSelfTestStageStartTimeMs_ =
        HAL_GetTick();

    buzzerPatternSequencer_.reset(
        HAL_GetTick());

    buzzerPatternSequencer_.update(
        BuzzerPattern::Off,
        HAL_GetTick());

    buzzerPwm_.setEnabled(
        false);

    reportBuzzerTimingState();

    if (watchdog_.refresh())
    {
        watchdogLastRefreshTimeMs_ =
            HAL_GetTick();
    }
}

void Application::run()
{
    const std::uint32_t currentTimeMs =
        HAL_GetTick();

    canBus_.service(
        currentTimeMs);

    if (canBus_.initialized())
    {
        processCanReceive();
    }

    updateRemoteCommunicationState();

    if (startupGraceActive_)
    {
        const bool connected =
            remoteVehicleStatus_.
                communicationState() ==
            RemoteCommunicationState::
                Connected;

        const bool graceExpired =
            (currentTimeMs -
             startupTimeMs_) >=
            StartupGracePeriodMs;

        if (connected ||
            graceExpired)
        {
            startupGraceActive_ =
                false;
        }
    }

    updateThermalControlState();

    if ((!startupGraceActive_) &&
        (!actuatorSelfTestActive_) &&
        (remoteVehicleStatus_.
             communicationState() ==
         RemoteCommunicationState::
             WaitingForData))
    {
        activeOutputCommand_ =
            actuatorCommandPolicy_.
                command();

        applyActiveOutputCommand();
    }

    updateUserButton(
        currentTimeMs);

    updateActuatorSelfTest(
        currentTimeMs);

    updateBuzzerPatternTiming();

    updateWatchdog(
        currentTimeMs);
}

void Application::processCanReceive()
{
    CanFrame frame{};

    while (canBus_.receive(frame))
    {
        const std::uint32_t currentTimeMs =
            HAL_GetTick();

        const bool accepted =
            remoteVehicleStatus_.processFrame(
                frame,
                currentTimeMs);

        if (accepted)
        {
            reportRemoteVehicleStatus();
        }
    }
}

void Application::
    updateRemoteCommunicationState()
{
    const std::uint32_t currentTimeMs =
        HAL_GetTick();

    remoteVehicleStatus_.
        updateCommunicationState(
            currentTimeMs);

    const RemoteCommunicationState
        currentState =
            remoteVehicleStatus_.
                communicationState();

    if ((!communicationStateInitialized_) ||
        (currentState !=
         previousCommunicationState_))
    {
        reportCommunicationState(
            currentState);

        previousCommunicationState_ =
            currentState;

        communicationStateInitialized_ =
            true;
    }
}

void Application::
    updateThermalControlState()
{
    thermalControlStateMachine_.update(
        remoteVehicleStatus_);

    const ThermalControlState currentState =
        thermalControlStateMachine_.
            state();

    if ((!thermalControlStateInitialized_) ||
        (currentState !=
         previousThermalControlState_))
    {
        reportThermalControlState(
            currentState);

        previousThermalControlState_ =
            currentState;

        thermalControlStateInitialized_ =
            true;

        actuatorCommandPolicy_.update(
            currentState);

        reportActuatorCommand();

        if (!actuatorSelfTestActive_)
        {
            const bool holdQuietStartupOutputs =
                startupGraceActive_ &&
                (remoteVehicleStatus_.
                     communicationState() ==
                 RemoteCommunicationState::
                     WaitingForData);

            if (!holdQuietStartupOutputs)
            {
                activeOutputCommand_ =
                    actuatorCommandPolicy_.
                        command();

                applyActiveOutputCommand();

                reportFanPwmState();
            }
        }
    }
}

void Application::
    updateUserButton(
        std::uint32_t currentTimeMs)
{
    const bool rawPressed =
        readUserButtonPressed();

    if (rawPressed !=
        userButtonRawPressed_)
    {
        userButtonRawPressed_ =
            rawPressed;

        userButtonRawChangeTimeMs_ =
            currentTimeMs;
    }

    if ((rawPressed !=
         userButtonDebouncedPressed_) &&
        ((currentTimeMs -
          userButtonRawChangeTimeMs_) >=
         UserButtonDebounceTimeMs))
    {
        userButtonDebouncedPressed_ =
            rawPressed;

        if (!userButtonDebouncedPressed_)
        {
            userButtonReleaseStartTimeMs_ =
                currentTimeMs;
        }
        else if (userButtonArmed_)
        {
            startActuatorSelfTest(
                currentTimeMs);
        }
    }

    if (!userButtonArmed_)
    {
        if (userButtonDebouncedPressed_)
        {
            userButtonReleaseStartTimeMs_ =
                currentTimeMs;

            return;
        }

        if ((currentTimeMs -
             userButtonReleaseStartTimeMs_) >=
            UserButtonStartupArmTimeMs)
        {
            userButtonArmed_ =
                true;
        }
    }
}

void Application::
    startActuatorSelfTest(
        std::uint32_t currentTimeMs)
{
    if (actuatorSelfTestActive_)
    {
        return;
    }

    actuatorSelfTestActive_ =
        true;

    actuatorSelfTestStage_ =
        0U;

    actuatorSelfTestStageStartTimeMs_ =
        currentTimeMs;

    activeOutputCommand_ =
        actuatorSelfTestCommand(
            actuatorSelfTestStage_);

    applyActiveOutputCommand();

    transmitText(
        "ACTUATOR SELF TEST START\r\n");

    reportActuatorSelfTestStage();
}

void Application::
    updateActuatorSelfTest(
        std::uint32_t currentTimeMs)
{
    if (!actuatorSelfTestActive_)
    {
        return;
    }

    const std::uint32_t elapsedTimeMs =
        currentTimeMs -
        actuatorSelfTestStageStartTimeMs_;

    if (elapsedTimeMs <
        ActuatorSelfTestStageTimeMs)
    {
        return;
    }

    ++actuatorSelfTestStage_;

    if (actuatorSelfTestStage_ >=
        ActuatorSelfTestStageCount)
    {
        actuatorSelfTestActive_ =
            false;

        activeOutputCommand_ =
            actuatorCommandPolicy_.
                command();

        applyActiveOutputCommand();

        transmitText(
            "ACTUATOR SELF TEST COMPLETE\r\n");

        reportActuatorCommand();

        reportFanPwmState();

        return;
    }

    actuatorSelfTestStageStartTimeMs_ =
        currentTimeMs;

    activeOutputCommand_ =
        actuatorSelfTestCommand(
            actuatorSelfTestStage_);

    applyActiveOutputCommand();

    reportActuatorSelfTestStage();
}

void Application::
    applyActiveOutputCommand()
{
    rgbLed_.setColor(
        activeOutputCommand_.
            warningColor);

    fanPwm_.setDutyPercent(
        activeOutputCommand_.
            coolingDutyPercent);
}

void Application::
    updateBuzzerPatternTiming()
{
    buzzerPatternSequencer_.update(
        activeOutputCommand_.
            buzzerPattern,
        HAL_GetTick());

    buzzerPwm_.setEnabled(
        buzzerPatternSequencer_.
            outputActive());
}

void Application::updateWatchdog(
    std::uint32_t currentTimeMs)
{
    if (WatchdogResetTestEnabled)
    {
        return;
    }

    const std::uint32_t elapsedTimeMs =
        currentTimeMs -
        watchdogLastRefreshTimeMs_;

    if (elapsedTimeMs <
        WatchdogRefreshPeriodMs)
    {
        return;
    }

    const bool refreshed =
        watchdog_.refresh();

    watchdogLastRefreshTimeMs_ =
        currentTimeMs;

    if (!refreshed)
    {
        transmitText(
            "BOARD2 ERROR WATCHDOG REFRESH FAILED\r\n");
    }
}

bool Application::
    readUserButtonPressed() const
{
    return
        HAL_GPIO_ReadPin(
            B1_GPIO_Port,
            B1_Pin) ==
        GPIO_PIN_RESET;
}

ActuatorCommand
Application::actuatorSelfTestCommand(
    std::uint8_t stage) const
{
    ActuatorCommand command{};

    switch (stage)
    {
        case 0U:
        {
            command.coolingDutyPercent =
                0U;
            command.warningColor =
                WarningColor::Green;
            command.buzzerPattern =
                BuzzerPattern::Off;
            break;
        }

        case 1U:
        {
            command.coolingDutyPercent =
                40U;
            command.warningColor =
                WarningColor::Blue;
            command.buzzerPattern =
                BuzzerPattern::Off;
            break;
        }

        case 2U:
        {
            command.coolingDutyPercent =
                70U;
            command.warningColor =
                WarningColor::Orange;
            command.buzzerPattern =
                BuzzerPattern::SlowBeep;
            break;
        }

        case 3U:
        {
            command.coolingDutyPercent =
                100U;
            command.warningColor =
                WarningColor::Red;
            command.buzzerPattern =
                BuzzerPattern::FastBeep;
            break;
        }

        default:
        {
            command.coolingDutyPercent =
                100U;
            command.warningColor =
                WarningColor::Magenta;
            command.buzzerPattern =
                BuzzerPattern::Fault;
            break;
        }
    }

    return command;
}

void Application::
    reportActuatorSelfTestStage()
{
    char message[192]{};

    const int length =
        std::snprintf(
            message,
            sizeof(message),
            "actuator_self_test_stage=%u "
            "cooling_duty_pct=%u "
            "led=%s "
            "buzzer=%s\r\n",
            static_cast<unsigned int>(
                actuatorSelfTestStage_),
            static_cast<unsigned int>(
                activeOutputCommand_.
                    coolingDutyPercent),
            warningColorName(
                activeOutputCommand_.
                    warningColor),
            buzzerPatternName(
                activeOutputCommand_.
                    buzzerPattern));

    if ((length <= 0) ||
        (length >=
         static_cast<int>(
             sizeof(message))))
    {
        return;
    }

    HAL_UART_Transmit(
        &huart2,
        reinterpret_cast<std::uint8_t*>(
            message),
        static_cast<std::uint16_t>(
            length),
        UartTimeoutMs);
}

void Application::
    reportRemoteVehicleStatus()
{
    char message[256]{};

    const int length =
        std::snprintf(
            message,
            sizeof(message),
            "can_rx_count=%lu "
            "remote_healthy=%u "
            "remote_temp_valid=%u "
            "remote_sensor_a=%u "
            "remote_sensor_b=%u "
            "remote_temp_dC=%d "
            "remote_fault_mask=0x%08lX\r\n",
            static_cast<unsigned long>(
                remoteVehicleStatus_.
                    validFrameCount()),
            remoteVehicleStatus_.
                    systemHealthy()
                ? 1U
                : 0U,
            remoteVehicleStatus_.
                    temperatureValid()
                ? 1U
                : 0U,
            remoteVehicleStatus_.
                    sensorAAvailable()
                ? 1U
                : 0U,
            remoteVehicleStatus_.
                    sensorBAvailable()
                ? 1U
                : 0U,
            static_cast<int>(
                remoteVehicleStatus_.
                    temperatureDeciCelsius()),
            static_cast<unsigned long>(
                remoteVehicleStatus_.
                    faultMask()));

    if ((length <= 0) ||
        (length >=
         static_cast<int>(
             sizeof(message))))
    {
        return;
    }

    HAL_UART_Transmit(
        &huart2,
        reinterpret_cast<std::uint8_t*>(
            message),
        static_cast<std::uint16_t>(
            length),
        UartTimeoutMs);
}

void Application::reportCommunicationState(
    RemoteCommunicationState state)
{
    switch (state)
    {
        case RemoteCommunicationState::
            WaitingForData:
        {
            transmitText(
                "remote_can_state="
                "WAITING_FOR_DATA\r\n");
            break;
        }

        case RemoteCommunicationState::
            Connected:
        {
            transmitText(
                "remote_can_state="
                "CONNECTED\r\n");
            break;
        }

        case RemoteCommunicationState::
            CommunicationLost:
        {
            transmitText(
                "remote_can_state="
                "COMMUNICATION_LOST\r\n");
            break;
        }
    }
}

void Application::
    reportThermalControlState(
        ThermalControlState state)
{
    switch (state)
    {
        case ThermalControlState::Normal:
        {
            transmitText(
                "thermal_state=NORMAL\r\n");
            break;
        }

        case ThermalControlState::Warm:
        {
            transmitText(
                "thermal_state=WARM\r\n");
            break;
        }

        case ThermalControlState::Cooling:
        {
            transmitText(
                "thermal_state=COOLING\r\n");
            break;
        }

        case ThermalControlState::High:
        {
            transmitText(
                "thermal_state=HIGH\r\n");
            break;
        }

        case ThermalControlState::Critical:
        {
            transmitText(
                "thermal_state=CRITICAL\r\n");
            break;
        }

        case ThermalControlState::Safe:
        {
            transmitText(
                "thermal_state=SAFE\r\n");
            break;
        }
    }
}

void Application::reportActuatorCommand()
{
    const ActuatorCommand& command =
        actuatorCommandPolicy_.command();

    char message[192]{};

    const int length =
        std::snprintf(
            message,
            sizeof(message),
            "actuator_cooling_duty_pct=%u "
            "actuator_led=%s "
            "actuator_buzzer=%s\r\n",
            static_cast<unsigned int>(
                command.coolingDutyPercent),
            warningColorName(
                command.warningColor),
            buzzerPatternName(
                command.buzzerPattern));

    if ((length <= 0) ||
        (length >=
         static_cast<int>(
             sizeof(message))))
    {
        return;
    }

    HAL_UART_Transmit(
        &huart2,
        reinterpret_cast<std::uint8_t*>(
            message),
        static_cast<std::uint16_t>(
            length),
        UartTimeoutMs);
}

void Application::reportBuzzerTimingState()
{
    const char* outputState =
        buzzerPatternSequencer_.
                outputActive()
            ? "ON"
            : "OFF";

    char message[128]{};

    const int length =
        std::snprintf(
            message,
            sizeof(message),
            "buzzer_timing_pattern=%s "
            "buzzer_output=%s\r\n",
            buzzerPatternName(
                buzzerPatternSequencer_.
                    activePattern()),
            outputState);

    if ((length <= 0) ||
        (length >=
         static_cast<int>(
             sizeof(message))))
    {
        return;
    }

    HAL_UART_Transmit(
        &huart2,
        reinterpret_cast<std::uint8_t*>(
            message),
        static_cast<std::uint16_t>(
            length),
        UartTimeoutMs);
}

void Application::reportFanPwmState()
{
    char message[96]{};

    const int length =
        std::snprintf(
            message,
            sizeof(message),
            "fan_pwm_duty_pct=%u\r\n",
            static_cast<unsigned int>(
                fanPwm_.dutyPercent()));

    if ((length <= 0) ||
        (length >=
         static_cast<int>(
             sizeof(message))))
    {
        return;
    }

    HAL_UART_Transmit(
        &huart2,
        reinterpret_cast<std::uint8_t*>(
            message),
        static_cast<std::uint16_t>(
            length),
        UartTimeoutMs);
}

void Application::runRemoteStatusSelfTest()
{
    RemoteVehicleStatus testStatus{};

    testStatus.reset();

    CanFrame testFrame =
        buildTestVehicleHealthFrame(
            true,
            247);

    constexpr std::uint32_t
        TestReceiveTimeMs = 1000U;

    const bool frameAccepted =
        testStatus.processFrame(
            testFrame,
            TestReceiveTimeMs);

    const bool decodedCorrectly =
        frameAccepted &&
        testStatus.hasReceivedValidFrame() &&
        testStatus.systemHealthy() &&
        testStatus.temperatureValid() &&
        testStatus.sensorAAvailable() &&
        testStatus.sensorBAvailable() &&
        (testStatus.temperatureDeciCelsius() ==
         247) &&
        (testStatus.faultMask() ==
         0U) &&
        (testStatus.communicationState() ==
         RemoteCommunicationState::
             Connected);

    constexpr std::uint32_t
        TestTimeoutTimeMs =
            TestReceiveTimeMs +
            RemoteVehicleStatus::
                CommunicationTimeoutMs +
            1U;

    testStatus.updateCommunicationState(
        TestTimeoutTimeMs);

    const bool timeoutCorrect =
        testStatus.communicationState() ==
        RemoteCommunicationState::
            CommunicationLost;

    if (decodedCorrectly &&
        timeoutCorrect)
    {
        transmitText(
            "REMOTE STATUS SELF TEST PASSED\r\n");
    }
    else
    {
        transmitText(
            "REMOTE STATUS SELF TEST FAILED\r\n");
    }
}

void Application::runThermalControlSelfTest()
{
    RemoteVehicleStatus testStatus{};

    ThermalControlStateMachine testStateMachine{};

    testStatus.reset();

    testStateMachine.reset();

    std::uint32_t testTimeMs = 1000U;

    bool passed = true;

    CanFrame testFrame =
        buildTestVehicleHealthFrame(
            true,
            250);

    passed &=
        testStatus.processFrame(
            testFrame,
            testTimeMs);

    testStateMachine.update(
        testStatus);

    passed &=
        testStateMachine.state() ==
        ThermalControlState::Normal;

    ++testTimeMs;

    testFrame =
        buildTestVehicleHealthFrame(
            true,
            350);

    passed &=
        testStatus.processFrame(
            testFrame,
            testTimeMs);

    testStateMachine.update(
        testStatus);

    passed &=
        testStateMachine.state() ==
        ThermalControlState::Warm;

    ++testTimeMs;

    testFrame =
        buildTestVehicleHealthFrame(
            true,
            450);

    passed &=
        testStatus.processFrame(
            testFrame,
            testTimeMs);

    testStateMachine.update(
        testStatus);

    passed &=
        testStateMachine.state() ==
        ThermalControlState::Cooling;

    ++testTimeMs;

    testFrame =
        buildTestVehicleHealthFrame(
            true,
            550);

    passed &=
        testStatus.processFrame(
            testFrame,
            testTimeMs);

    testStateMachine.update(
        testStatus);

    passed &=
        testStateMachine.state() ==
        ThermalControlState::High;

    ++testTimeMs;

    testFrame =
        buildTestVehicleHealthFrame(
            true,
            600);

    passed &=
        testStatus.processFrame(
            testFrame,
            testTimeMs);

    testStateMachine.update(
        testStatus);

    passed &=
        testStateMachine.state() ==
        ThermalControlState::Critical;

    ++testTimeMs;

    testFrame =
        buildTestVehicleHealthFrame(
            false,
            600);

    passed &=
        testStatus.processFrame(
            testFrame,
            testTimeMs);

    testStateMachine.update(
        testStatus);

    passed &=
        testStateMachine.state() ==
        ThermalControlState::Safe;

    ++testTimeMs;

    testFrame =
        buildTestVehicleHealthFrame(
            true,
            450);

    passed &=
        testStatus.processFrame(
            testFrame,
            testTimeMs);

    testStateMachine.update(
        testStatus);

    passed &=
        testStateMachine.state() ==
        ThermalControlState::Cooling;

    testStatus.updateCommunicationState(
        testTimeMs +
        RemoteVehicleStatus::
            CommunicationTimeoutMs +
        1U);

    testStateMachine.update(
        testStatus);

    passed &=
        testStateMachine.state() ==
        ThermalControlState::Safe;

    if (passed)
    {
        transmitText(
            "THERMAL CONTROL SELF TEST PASSED\r\n");
    }
    else
    {
        transmitText(
            "THERMAL CONTROL SELF TEST FAILED\r\n");
    }
}

void Application::runActuatorCommandSelfTest()
{
    ActuatorCommandPolicy testPolicy{};

    bool passed = true;

    testPolicy.reset();

    passed &=
        actuatorCommandMatches(
            testPolicy.command(),
            100U,
            WarningColor::Magenta,
            BuzzerPattern::Fault);

    testPolicy.update(
        ThermalControlState::Normal);

    passed &=
        actuatorCommandMatches(
            testPolicy.command(),
            0U,
            WarningColor::Green,
            BuzzerPattern::Off);

    testPolicy.update(
        ThermalControlState::Warm);

    passed &=
        actuatorCommandMatches(
            testPolicy.command(),
            0U,
            WarningColor::Yellow,
            BuzzerPattern::Off);

    testPolicy.update(
        ThermalControlState::Cooling);

    passed &=
        actuatorCommandMatches(
            testPolicy.command(),
            40U,
            WarningColor::Blue,
            BuzzerPattern::Off);

    testPolicy.update(
        ThermalControlState::High);

    passed &=
        actuatorCommandMatches(
            testPolicy.command(),
            70U,
            WarningColor::Orange,
            BuzzerPattern::SlowBeep);

    testPolicy.update(
        ThermalControlState::Critical);

    passed &=
        actuatorCommandMatches(
            testPolicy.command(),
            100U,
            WarningColor::Red,
            BuzzerPattern::FastBeep);

    testPolicy.update(
        ThermalControlState::Safe);

    passed &=
        actuatorCommandMatches(
            testPolicy.command(),
            100U,
            WarningColor::Magenta,
            BuzzerPattern::Fault);

    if (passed)
    {
        transmitText(
            "ACTUATOR COMMAND SELF TEST PASSED\r\n");
    }
    else
    {
        transmitText(
            "ACTUATOR COMMAND SELF TEST FAILED\r\n");
    }
}

void Application::runBuzzerTimingSelfTest()
{
    BuzzerPatternSequencer testSequencer{};

    bool passed = true;

    testSequencer.reset(0U);

    testSequencer.update(
        BuzzerPattern::Off,
        100U);

    passed &=
        !testSequencer.outputActive();

    constexpr std::uint32_t
        SlowStartTimeMs = 1000U;

    testSequencer.update(
        BuzzerPattern::SlowBeep,
        SlowStartTimeMs);

    passed &=
        testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::SlowBeep,
        SlowStartTimeMs + 249U);

    passed &=
        testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::SlowBeep,
        SlowStartTimeMs + 250U);

    passed &=
        !testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::SlowBeep,
        SlowStartTimeMs + 999U);

    passed &=
        !testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::SlowBeep,
        SlowStartTimeMs + 1000U);

    passed &=
        testSequencer.outputActive();

    constexpr std::uint32_t
        FastStartTimeMs = 3000U;

    testSequencer.update(
        BuzzerPattern::FastBeep,
        FastStartTimeMs);

    passed &=
        testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::FastBeep,
        FastStartTimeMs + 199U);

    passed &=
        testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::FastBeep,
        FastStartTimeMs + 200U);

    passed &=
        !testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::FastBeep,
        FastStartTimeMs + 400U);

    passed &=
        testSequencer.outputActive();

    constexpr std::uint32_t
        FaultStartTimeMs = 5000U;

    testSequencer.update(
        BuzzerPattern::Fault,
        FaultStartTimeMs);

    passed &=
        testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::Fault,
        FaultStartTimeMs + 149U);

    passed &=
        testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::Fault,
        FaultStartTimeMs + 150U);

    passed &=
        !testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::Fault,
        FaultStartTimeMs + 299U);

    passed &=
        !testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::Fault,
        FaultStartTimeMs + 300U);

    passed &=
        testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::Fault,
        FaultStartTimeMs + 449U);

    passed &=
        testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::Fault,
        FaultStartTimeMs + 450U);

    passed &=
        !testSequencer.outputActive();

    testSequencer.update(
        BuzzerPattern::Fault,
        FaultStartTimeMs + 1500U);

    passed &=
        testSequencer.outputActive();

    constexpr std::uint32_t
        WraparoundStartTimeMs =
            0xFFFFFF00U;

    testSequencer.reset(
        WraparoundStartTimeMs);

    testSequencer.update(
        BuzzerPattern::SlowBeep,
        WraparoundStartTimeMs);

    testSequencer.update(
        BuzzerPattern::SlowBeep,
        0x00000020U);

    passed &=
        !testSequencer.outputActive();

    if (passed)
    {
        transmitText(
            "BUZZER TIMING SELF TEST PASSED\r\n");
    }
    else
    {
        transmitText(
            "BUZZER TIMING SELF TEST FAILED\r\n");
    }
}

void Application::runRgbMappingSelfTest()
{
    bool passed = true;

    passed &=
        rgbIntensityMatches(
            RgbLedPwm::intensityForColor(
                WarningColor::Green),
            0U,
            100U,
            0U);

    passed &=
        rgbIntensityMatches(
            RgbLedPwm::intensityForColor(
                WarningColor::Yellow),
            100U,
            25U,
            0U);

    passed &=
        rgbIntensityMatches(
            RgbLedPwm::intensityForColor(
                WarningColor::Blue),
            0U,
            0U,
            100U);

    passed &=
        rgbIntensityMatches(
            RgbLedPwm::intensityForColor(
                WarningColor::Orange),
            100U,
            5U,
            0U);

    passed &=
        rgbIntensityMatches(
            RgbLedPwm::intensityForColor(
                WarningColor::Red),
            100U,
            0U,
            0U);

    passed &=
        rgbIntensityMatches(
            RgbLedPwm::intensityForColor(
                WarningColor::Magenta),
            100U,
            0U,
            80U);

    if (passed)
    {
        transmitText(
            "RGB MAPPING SELF TEST PASSED\r\n");
    }
    else
    {
        transmitText(
            "RGB MAPPING SELF TEST FAILED\r\n");
    }
}

void Application::runRgbHardwareSelfTest()
{
    if (!rgbLed_.initialized())
    {
        transmitText(
            "RGB HARDWARE SELF TEST NOT AVAILABLE\r\n");

        return;
    }

    rgbLed_.setColor(
        WarningColor::Green);

    HAL_Delay(
        RgbSelfTestColorTimeMs);

    rgbLed_.setColor(
        WarningColor::Yellow);

    HAL_Delay(
        RgbSelfTestColorTimeMs);

    rgbLed_.setColor(
        WarningColor::Blue);

    HAL_Delay(
        RgbSelfTestColorTimeMs);

    rgbLed_.setColor(
        WarningColor::Orange);

    HAL_Delay(
        RgbSelfTestColorTimeMs);

    rgbLed_.setColor(
        WarningColor::Red);

    HAL_Delay(
        RgbSelfTestColorTimeMs);

    rgbLed_.setColor(
        WarningColor::Magenta);

    HAL_Delay(
        RgbSelfTestColorTimeMs);

    rgbLed_.setColor(
        actuatorCommandPolicy_.
            command().
            warningColor);

    transmitText(
        "RGB HARDWARE SELF TEST COMPLETE\r\n");
}

void Application::runFanHardwareSelfTest()
{
    if (!fanPwm_.initialized())
    {
        transmitText(
            "FAN HARDWARE SELF TEST NOT AVAILABLE\r\n");

        return;
    }

    transmitText(
        "FAN HARDWARE SELF TEST START\r\n");

    fanPwm_.setDutyPercent(100U);

    HAL_Delay(
        FanSelfTestDutyTimeMs);

    fanPwm_.setDutyPercent(70U);

    HAL_Delay(
        FanSelfTestDutyTimeMs);

    fanPwm_.setDutyPercent(40U);

    HAL_Delay(
        FanSelfTestDutyTimeMs);

    fanPwm_.setDutyPercent(0U);

    HAL_Delay(
        FanSelfTestOffTimeMs);

    fanPwm_.setDutyPercent(
        actuatorCommandPolicy_.
            command().
            coolingDutyPercent);

    transmitText(
        "FAN HARDWARE SELF TEST COMPLETE\r\n");
}

void Application::transmitText(
    const char* text)
{
    if (text == nullptr)
    {
        return;
    }

    std::uint16_t length = 0U;

    while ((text[length] != '\0') &&
           (length <
            static_cast<std::uint16_t>(
                512U)))
    {
        ++length;
    }

    if (length == 0U)
    {
        return;
    }

    HAL_UART_Transmit(
        &huart2,
        reinterpret_cast<std::uint8_t*>(
            const_cast<char*>(text)),
        length,
        UartTimeoutMs);
}
