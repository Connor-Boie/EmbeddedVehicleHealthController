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

}

Application::Application()
    : canBus_{&hcan1}
{
}

void Application::initialize()
{
    remoteVehicleStatus_.reset();

    communicationStateInitialized_ =
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

    reportCommunicationState(
        remoteVehicleStatus_.
            communicationState());

    previousCommunicationState_ =
        remoteVehicleStatus_.
            communicationState();

    communicationStateInitialized_ =
        true;
}

void Application::run()
{
    processCanReceive();

    updateRemoteCommunicationState();
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

void Application::runRemoteStatusSelfTest()
{
    RemoteVehicleStatus testStatus{};

    testStatus.reset();

    CanFrame testFrame{};

    testFrame.id =
        CanProtocol::MessageId::
            VehicleHealthStatus;

    testFrame.length =
        CanProtocol::
            VehicleHealthStatus::
                PayloadLength;

    testFrame.data[
        CanProtocol::
            VehicleHealthStatus::
                ProtocolVersionIndex] =
        CanProtocol::ProtocolVersion;

    testFrame.data[
        CanProtocol::
            VehicleHealthStatus::
                StatusFlagsIndex] =
        CanProtocol::
            VehicleHealthStatus::
                SystemHealthyFlag |
        CanProtocol::
            VehicleHealthStatus::
                SelectedTemperatureValidFlag |
        CanProtocol::
            VehicleHealthStatus::
                SensorAAvailableFlag |
        CanProtocol::
            VehicleHealthStatus::
                SensorBAvailableFlag;

    constexpr std::int16_t
        TestTemperatureDeciCelsius = 247;

    const std::uint16_t
        testTemperatureUnsigned =
            static_cast<std::uint16_t>(
                TestTemperatureDeciCelsius);

    testFrame.data[
        CanProtocol::
            VehicleHealthStatus::
                TemperatureLowByteIndex] =
        static_cast<std::uint8_t>(
            testTemperatureUnsigned &
            0xFFU);

    testFrame.data[
        CanProtocol::
            VehicleHealthStatus::
                TemperatureHighByteIndex] =
        static_cast<std::uint8_t>(
            (testTemperatureUnsigned >>
             8U) &
            0xFFU);

    constexpr std::uint32_t
        TestFaultMask = 0x00000020U;

    testFrame.data[
        CanProtocol::
            VehicleHealthStatus::
                FaultMaskByte0Index] =
        static_cast<std::uint8_t>(
            TestFaultMask &
            0xFFU);

    testFrame.data[
        CanProtocol::
            VehicleHealthStatus::
                FaultMaskByte1Index] =
        static_cast<std::uint8_t>(
            (TestFaultMask >>
             8U) &
            0xFFU);

    testFrame.data[
        CanProtocol::
            VehicleHealthStatus::
                FaultMaskByte2Index] =
        static_cast<std::uint8_t>(
            (TestFaultMask >>
             16U) &
            0xFFU);

    testFrame.data[
        CanProtocol::
            VehicleHealthStatus::
                FaultMaskByte3Index] =
        static_cast<std::uint8_t>(
            (TestFaultMask >>
             24U) &
            0xFFU);

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
         TestTemperatureDeciCelsius) &&
        (testStatus.faultMask() ==
         TestFaultMask) &&
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
