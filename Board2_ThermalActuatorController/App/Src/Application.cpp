#include "Application.hpp"

#include "CanProtocol.hpp"
#include "main.h"

#include <cstdio>

extern "C"
{
extern CAN_HandleTypeDef hcan1;
extern UART_HandleTypeDef huart2;
}

namespace
{

constexpr std::uint32_t
    UartTimeoutMs = 100U;

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

    const std::uint16_t
        temperatureUnsigned =
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

}

Application::Application()
    : canBus_{&hcan1}
{
}

void Application::initialize()
{
    remoteVehicleStatus_.reset();

    thermalControlStateMachine_.reset();

    actuatorCommandPolicy_.reset();

    communicationStateInitialized_ =
        false;

    thermalControlStateInitialized_ =
        false;

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

    runRemoteStatusSelfTest();

    runThermalControlSelfTest();

    runActuatorCommandSelfTest();

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
}

void Application::run()
{
    if (canBus_.initialized())
    {
        processCanReceive();
    }

    updateRemoteCommunicationState();

    updateThermalControlState();
}

void Application::processCanReceive()
{
    CanFrame frame{};

    while (canBus_.receive(frame))
    {
        const std::uint32_t
            currentTimeMs =
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

    const ThermalControlState
        currentState =
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
    }
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

    ThermalControlStateMachine
        testStateMachine{};

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
