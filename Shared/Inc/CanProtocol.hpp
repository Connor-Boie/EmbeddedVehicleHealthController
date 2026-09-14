#ifndef CAN_PROTOCOL_HPP
#define CAN_PROTOCOL_HPP

#include <cstdint>

namespace CanProtocol
{

    constexpr std::uint8_t ProtocolVersion = 1U;

    namespace MessageId
    {

        constexpr std::uint32_t
            VehicleHealthStatus = 0x100U;

        constexpr std::uint32_t
            ThermalActuatorStatus = 0x101U;

    }

    namespace VehicleHealthStatus
    {

        constexpr std::uint8_t PayloadLength = 8U;

        constexpr std::uint8_t
            ProtocolVersionIndex = 0U;

        constexpr std::uint8_t
            StatusFlagsIndex = 1U;

        constexpr std::uint8_t
            TemperatureLowByteIndex = 2U;

        constexpr std::uint8_t
            TemperatureHighByteIndex = 3U;

        constexpr std::uint8_t
            FaultMaskByte0Index = 4U;

        constexpr std::uint8_t
            FaultMaskByte1Index = 5U;

        constexpr std::uint8_t
            FaultMaskByte2Index = 6U;

        constexpr std::uint8_t
            FaultMaskByte3Index = 7U;

        constexpr std::uint8_t
            SystemHealthyFlag = 0x01U;

        constexpr std::uint8_t
            SelectedTemperatureValidFlag = 0x02U;

        constexpr std::uint8_t
            SensorAAvailableFlag = 0x04U;

        constexpr std::uint8_t
            SensorBAvailableFlag = 0x08U;

        constexpr std::int16_t
            InvalidTemperatureDeciCelsius =
                static_cast<std::int16_t>(-32768);

    }

    namespace ThermalActuatorStatus
    {

        constexpr std::uint8_t PayloadLength = 8U;

        constexpr std::uint8_t
            ProtocolVersionIndex = 0U;

        constexpr std::uint8_t
            StatusFlagsIndex = 1U;

        constexpr std::uint8_t
            ThermalStateIndex = 2U;

        constexpr std::uint8_t
            CoolingDutyPercentIndex = 3U;

        constexpr std::uint8_t
            WarningColorIndex = 4U;

        constexpr std::uint8_t
            BuzzerPatternIndex = 5U;

        constexpr std::uint8_t
            Reserved0Index = 6U;

        constexpr std::uint8_t
            Reserved1Index = 7U;

        constexpr std::uint8_t
            ControllerOperationalFlag = 0x01U;

        constexpr std::uint8_t
            VehicleDataConnectedFlag = 0x02U;

        constexpr std::uint8_t
            SelfTestActiveFlag = 0x04U;

        constexpr std::uint8_t
            SafeStateFlag = 0x08U;

        namespace ThermalState
        {

            constexpr std::uint8_t Normal = 0U;
            constexpr std::uint8_t Warm = 1U;
            constexpr std::uint8_t Cooling = 2U;
            constexpr std::uint8_t High = 3U;
            constexpr std::uint8_t Critical = 4U;
            constexpr std::uint8_t Safe = 5U;

        }

        namespace WarningColor
        {

            constexpr std::uint8_t Green = 0U;
            constexpr std::uint8_t Yellow = 1U;
            constexpr std::uint8_t Blue = 2U;
            constexpr std::uint8_t Orange = 3U;
            constexpr std::uint8_t Red = 4U;
            constexpr std::uint8_t Magenta = 5U;

        }

        namespace BuzzerPattern
        {

            constexpr std::uint8_t Off = 0U;
            constexpr std::uint8_t SlowBeep = 1U;
            constexpr std::uint8_t FastBeep = 2U;
            constexpr std::uint8_t Fault = 3U;

        }

    }

}

#endif
