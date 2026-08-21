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

}

#endif
