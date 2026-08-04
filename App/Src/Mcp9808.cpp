#include "Mcp9808.hpp"

Mcp9808::Mcp9808(
    I2C_HandleTypeDef* i2c,
    std::uint8_t address7Bit)
    : i2c_{i2c},
      address7Bit_{address7Bit}
{
}

bool Mcp9808::initialize()
{
    available_ = false;

    if (i2c_ == nullptr)
    {
        ++failureCount_;
        return false;
    }

    const std::uint16_t halAddress =
        static_cast<std::uint16_t>(address7Bit_) << 1U;

    const HAL_StatusTypeDef readyStatus =
        HAL_I2C_IsDeviceReady(
            i2c_,
            halAddress,
            DeviceReadyTrials,
            CommunicationTimeoutMs);

    if (readyStatus != HAL_OK)
    {
        ++failureCount_;
        return false;
    }

    std::uint16_t manufacturerId = 0U;

    if (!readRegister16(
        ManufacturerIdRegister,
        manufacturerId))
    {
        return false;
    }

    if (manufacturerId != ExpectedManufacturerId)
    {
        ++failureCount_;
        return false;
    }

    std::uint16_t deviceIdAndRevision = 0U;

    if (!readRegister16(
        DeviceIdRegister,
        deviceIdAndRevision))
    {
        return false;
    }

    const std::uint8_t deviceId =
        static_cast<std::uint8_t>(
            deviceIdAndRevision >> 8U);

    if (deviceId != ExpectedDeviceId)
    {
        ++failureCount_;
        return false;
    }

    available_ = true;
    return true;
}

bool Mcp9808::readTemperature()
{
    std::uint16_t rawTemperature = 0U;

    if (!readRegister16(
        AmbientTemperatureRegister,
        rawTemperature))
    {
        available_ = false;
        return false;
    }

    temperatureMilliCelsius_ =
        convertRawTemperatureToMilliCelsius(
            rawTemperature);

    available_ = true;
    ++successfulReadCount_;

    return true;
}

bool Mcp9808::available() const
{
    return available_;
}

std::uint8_t Mcp9808::address7Bit() const
{
    return address7Bit_;
}

std::int32_t Mcp9808::temperatureMilliCelsius() const
{
    return temperatureMilliCelsius_;
}

std::uint32_t Mcp9808::successfulReadCount() const
{
    return successfulReadCount_;
}

std::uint32_t Mcp9808::failureCount() const
{
    return failureCount_;
}

bool Mcp9808::readRegister16(
    std::uint8_t registerAddress,
    std::uint16_t& value)
{
    if (i2c_ == nullptr)
    {
        ++failureCount_;
        return false;
    }

    std::uint8_t receivedBytes[2]{};

    const std::uint16_t halAddress =
        static_cast<std::uint16_t>(address7Bit_) << 1U;

    const HAL_StatusTypeDef readStatus =
        HAL_I2C_Mem_Read(
            i2c_,
            halAddress,
            registerAddress,
            I2C_MEMADD_SIZE_8BIT,
            receivedBytes,
            2U,
            CommunicationTimeoutMs);

    if (readStatus != HAL_OK)
    {
        ++failureCount_;
        return false;
    }

    value =
        static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(
                receivedBytes[0])
            << 8U) |
        static_cast<std::uint16_t>(
            receivedBytes[1]);

    return true;
}

std::int32_t
Mcp9808::convertRawTemperatureToMilliCelsius(
    std::uint16_t rawValue)
{
    constexpr std::uint16_t TemperatureDataMask = 0x0FFFU;
    constexpr std::uint16_t SignBitMask = 0x1000U;
    constexpr std::int32_t RawUnitsPerFullRange = 4096;

    std::int32_t signedSixteenths =
        static_cast<std::int32_t>(
            rawValue & TemperatureDataMask);

    if ((rawValue & SignBitMask) != 0U)
    {
        signedSixteenths -= RawUnitsPerFullRange;
    }

    return (signedSixteenths * 625) / 10;
}
