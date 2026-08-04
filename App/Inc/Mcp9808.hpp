#ifndef MCP9808_HPP
#define MCP9808_HPP

#include "stm32f4xx_hal.h"

#include <cstdint>

class Mcp9808
{
public:
    Mcp9808(
        I2C_HandleTypeDef* i2c,
        std::uint8_t address7Bit);

    [[nodiscard]] bool initialize();
    [[nodiscard]] bool readTemperature();

    [[nodiscard]] bool available() const;

    [[nodiscard]] std::uint8_t address7Bit() const;
    [[nodiscard]] std::int32_t temperatureMilliCelsius() const;

    [[nodiscard]] std::uint32_t successfulReadCount() const;
    [[nodiscard]] std::uint32_t failureCount() const;

private:
    [[nodiscard]] bool readRegister16(
        std::uint8_t registerAddress,
        std::uint16_t& value);

    [[nodiscard]] static std::int32_t
        convertRawTemperatureToMilliCelsius(
            std::uint16_t rawValue);

    static constexpr std::uint8_t AmbientTemperatureRegister = 0x05U;
    static constexpr std::uint8_t ManufacturerIdRegister = 0x06U;
    static constexpr std::uint8_t DeviceIdRegister = 0x07U;

    static constexpr std::uint16_t ExpectedManufacturerId = 0x0054U;
    static constexpr std::uint8_t ExpectedDeviceId = 0x04U;

    static constexpr std::uint32_t CommunicationTimeoutMs = 50U;
    static constexpr std::uint32_t DeviceReadyTrials = 3U;

    I2C_HandleTypeDef* i2c_;
    std::uint8_t address7Bit_;

    std::int32_t temperatureMilliCelsius_{0};

    std::uint32_t successfulReadCount_{0U};
    std::uint32_t failureCount_{0U};

    bool available_{false};
};

#endif
