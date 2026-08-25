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
    UartTransmitTimeoutMs = 100U;

constexpr std::size_t
    UartMessageCapacity = 256U;
}

Application::Application()
    : canBus_{
          &hcan1}
{
}

void Application::initialize()
{
    vehicleHealthReceiveCount_ = 0U;
    invalidVehicleHealthCount_ = 0U;

    const bool canInitialized =
        canBus_.initialize();

    sendStartupMessage(
        canInitialized);
}

void Application::run()
{
    processCanReceive();
}

void Application::processCanReceive()
{
    while (canBus_.messagePending())
    {
        CanFrame frame{};

        if (!canBus_.receive(frame))
        {
            sendText(
                "ERROR CAN RECEIVE FAILED");

            return;
        }

        if (frame.id ==
            CanProtocol::MessageId::
                VehicleHealthStatus)
        {
            processVehicleHealthStatus(
                frame);
        }
    }
}

void Application::processVehicleHealthStatus(
    const CanFrame& frame)
{
    if (frame.length !=
        CanProtocol::VehicleHealthStatus::
            PayloadLength)
    {
        ++invalidVehicleHealthCount_;

        sendText(
            "ERROR VEHICLE HEALTH LENGTH");

        return;
    }

    const std::uint8_t protocolVersion =
        frame.data[
            CanProtocol::
                VehicleHealthStatus::
                ProtocolVersionIndex];

    if (protocolVersion !=
        CanProtocol::ProtocolVersion)
    {
        ++invalidVehicleHealthCount_;

        sendText(
            "ERROR CAN PROTOCOL VERSION");

        return;
    }

    const std::uint8_t statusFlags =
        frame.data[
            CanProtocol::
                VehicleHealthStatus::
                StatusFlagsIndex];

    const bool systemHealthy =
        (statusFlags &
         CanProtocol::
             VehicleHealthStatus::
             SystemHealthyFlag) != 0U;

    const bool temperatureValid =
        (statusFlags &
         CanProtocol::
             VehicleHealthStatus::
             SelectedTemperatureValidFlag) != 0U;

    const bool sensorAAvailable =
        (statusFlags &
         CanProtocol::
             VehicleHealthStatus::
             SensorAAvailableFlag) != 0U;

    const bool sensorBAvailable =
        (statusFlags &
         CanProtocol::
             VehicleHealthStatus::
             SensorBAvailableFlag) != 0U;

    const std::uint16_t
        encodedTemperature =
            static_cast<std::uint16_t>(
                frame.data[
                    CanProtocol::
                        VehicleHealthStatus::
                        TemperatureLowByteIndex]) |
            static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(
                    frame.data[
                        CanProtocol::
                            VehicleHealthStatus::
                            TemperatureHighByteIndex])
                << 8U);

    const std::int16_t
        temperatureDeciCelsius =
            static_cast<std::int16_t>(
                encodedTemperature);

    const std::uint32_t faultMask =
        static_cast<std::uint32_t>(
            frame.data[
                CanProtocol::
                    VehicleHealthStatus::
                    FaultMaskByte0Index]) |
        (static_cast<std::uint32_t>(
            frame.data[
                CanProtocol::
                    VehicleHealthStatus::
                    FaultMaskByte1Index])
         << 8U) |
        (static_cast<std::uint32_t>(
            frame.data[
                CanProtocol::
                    VehicleHealthStatus::
                    FaultMaskByte2Index])
         << 16U) |
        (static_cast<std::uint32_t>(
            frame.data[
                CanProtocol::
                    VehicleHealthStatus::
                    FaultMaskByte3Index])
         << 24U);

    ++vehicleHealthReceiveCount_;

    sendVehicleHealthStatus(
        protocolVersion,
        systemHealthy,
        temperatureValid,
        sensorAAvailable,
        sensorBAvailable,
        temperatureDeciCelsius,
        faultMask);
}

void Application::sendStartupMessage(
    bool canInitialized)
{
    sendText(
        canInitialized
            ? "BOARD2 READY CAN INITIALIZED"
            : "BOARD2 ERROR CAN NOT INITIALIZED");
}

void Application::sendVehicleHealthStatus(
    std::uint8_t protocolVersion,
    bool systemHealthy,
    bool temperatureValid,
    bool sensorAAvailable,
    bool sensorBAvailable,
    std::int16_t temperatureDeciCelsius,
    std::uint32_t faultMask)
{
    char message[
        UartMessageCapacity]{};

    const int written =
        std::snprintf(
            message,
            sizeof(message),
            "can_rx_count=%lu "
            "protocol=%u "
            "remote_healthy=%u "
            "remote_temp_valid=%u "
            "remote_sensor_a=%u "
            "remote_sensor_b=%u "
            "remote_temp_dC=%d "
            "remote_fault_mask=0x%08lX",
            static_cast<unsigned long>(
                vehicleHealthReceiveCount_),
            static_cast<unsigned int>(
                protocolVersion),
            systemHealthy ? 1U : 0U,
            temperatureValid ? 1U : 0U,
            sensorAAvailable ? 1U : 0U,
            sensorBAvailable ? 1U : 0U,
            static_cast<int>(
                temperatureDeciCelsius),
            static_cast<unsigned long>(
                faultMask));

    if (written <= 0)
    {
        return;
    }

    sendText(message);
}

void Application::sendText(
    const char* text)
{
    if (text == nullptr)
    {
        return;
    }

    char message[
        UartMessageCapacity]{};

    const int written =
        std::snprintf(
            message,
            sizeof(message),
            "%s\r\n",
            text);

    if (written <= 0)
    {
        return;
    }

    std::size_t length =
        static_cast<std::size_t>(
            written);

    if (length >= sizeof(message))
    {
        length =
            sizeof(message) - 1U;
    }

    const HAL_StatusTypeDef status =
        HAL_UART_Transmit(
            &huart2,
            reinterpret_cast<std::uint8_t*>(
                message),
            static_cast<std::uint16_t>(
                length),
            UartTransmitTimeoutMs);

    static_cast<void>(
        status);
}
