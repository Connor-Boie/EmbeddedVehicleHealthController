#include "W25q64.hpp"

W25q64::W25q64(
    SPI_HandleTypeDef* spi,
    GPIO_TypeDef* chipSelectPort,
    std::uint16_t chipSelectPin)
    : spi_{spi},
      chipSelectPort_{chipSelectPort},
      chipSelectPin_{chipSelectPin}
{
}

bool W25q64::initialize()
{
    available_ = false;
    jedecId_ = 0U;

    if ((spi_ == nullptr) ||
        (chipSelectPort_ == nullptr))
    {
        ++failureCount_;
        return false;
    }

    deselect();

    if (!readJedecId())
    {
        return false;
    }

    const std::uint8_t manufacturerId =
        static_cast<std::uint8_t>(
            (jedecId_ >> 16U) & 0xFFU);

    const std::uint8_t capacityId =
        static_cast<std::uint8_t>(
            jedecId_ & 0xFFU);

    if ((manufacturerId !=
         ExpectedManufacturerId) ||
        (capacityId != ExpectedCapacityId))
    {
        ++failureCount_;
        return false;
    }

    available_ = true;
    return true;
}

bool W25q64::read(
    std::uint32_t address,
    std::uint8_t* data,
    std::size_t length)
{
    if ((data == nullptr) ||
        !validRange(address, length))
    {
        ++failureCount_;
        return false;
    }

    if (length == 0U)
    {
        return true;
    }

    if (!waitUntilReady())
    {
        return false;
    }

    const std::uint8_t command[4]{
        ReadDataCommand,
        static_cast<std::uint8_t>(
            (address >> 16U) & 0xFFU),
        static_cast<std::uint8_t>(
            (address >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(
            address & 0xFFU)
    };

    select();

    if (!transmit(command, sizeof(command)))
    {
        deselect();
        return false;
    }

    if (!receive(data, length))
    {
        deselect();
        return false;
    }

    deselect();

    available_ = true;
    return true;
}

bool W25q64::program(
    std::uint32_t address,
    const std::uint8_t* data,
    std::size_t length)
{
    if ((data == nullptr) ||
        !validRange(address, length))
    {
        ++failureCount_;
        return false;
    }

    if (length == 0U)
    {
        return true;
    }

    std::uint32_t currentAddress = address;
    std::size_t currentOffset = 0U;
    std::size_t remaining = length;

    while (remaining > 0U)
    {
        const std::uint32_t pageOffset =
            currentAddress % PageSizeBytes;

        const std::size_t bytesRemainingInPage =
            static_cast<std::size_t>(
                PageSizeBytes - pageOffset);

        const std::size_t chunkLength =
            (remaining < bytesRemainingInPage)
                ? remaining
                : bytesRemainingInPage;

        if (!waitUntilReady())
        {
            return false;
        }

        if (!writeEnable())
        {
            return false;
        }

        const std::uint8_t command[4]{
            PageProgramCommand,
            static_cast<std::uint8_t>(
                (currentAddress >> 16U) & 0xFFU),
            static_cast<std::uint8_t>(
                (currentAddress >> 8U) & 0xFFU),
            static_cast<std::uint8_t>(
                currentAddress & 0xFFU)
        };

        select();

        if (!transmit(command, sizeof(command)))
        {
            deselect();
            return false;
        }

        if (!transmit(
            &data[currentOffset],
            chunkLength))
        {
            deselect();
            return false;
        }

        deselect();

        if (!waitUntilReady())
        {
            return false;
        }

        currentAddress +=
            static_cast<std::uint32_t>(
                chunkLength);

        currentOffset += chunkLength;
        remaining -= chunkLength;
    }

    available_ = true;
    return true;
}

bool W25q64::eraseSector(
    std::uint32_t address)
{
    if (address >= CapacityBytes)
    {
        ++failureCount_;
        return false;
    }

    const std::uint32_t sectorAddress =
        address & ~(SectorSizeBytes - 1U);

    if (!waitUntilReady())
    {
        return false;
    }

    if (!writeEnable())
    {
        return false;
    }

    const std::uint8_t command[4]{
        SectorEraseCommand,
        static_cast<std::uint8_t>(
            (sectorAddress >> 16U) & 0xFFU),
        static_cast<std::uint8_t>(
            (sectorAddress >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(
            sectorAddress & 0xFFU)
    };

    select();

    const bool commandSent =
        transmit(command, sizeof(command));

    deselect();

    if (!commandSent)
    {
        return false;
    }

    if (!waitUntilReady())
    {
        return false;
    }

    available_ = true;
    return true;
}

bool W25q64::available() const
{
    return available_;
}

std::uint32_t W25q64::jedecId() const
{
    return jedecId_;
}

std::uint32_t W25q64::failureCount() const
{
    return failureCount_;
}

bool W25q64::readJedecId()
{
    const std::uint8_t command =
        ReadJedecIdCommand;

    std::uint8_t idBytes[3]{};

    select();

    if (!transmit(&command, 1U))
    {
        deselect();
        return false;
    }

    if (!receive(idBytes, sizeof(idBytes)))
    {
        deselect();
        return false;
    }

    deselect();

    jedecId_ =
        (static_cast<std::uint32_t>(
             idBytes[0]) << 16U) |
        (static_cast<std::uint32_t>(
             idBytes[1]) << 8U) |
        static_cast<std::uint32_t>(
            idBytes[2]);

    return true;
}

bool W25q64::readStatusRegister1(
    std::uint8_t& status)
{
    const std::uint8_t command =
        ReadStatusRegister1Command;

    select();

    if (!transmit(&command, 1U))
    {
        deselect();
        return false;
    }

    if (!receive(&status, 1U))
    {
        deselect();
        return false;
    }

    deselect();
    return true;
}

bool W25q64::writeEnable()
{
    const std::uint8_t command =
        WriteEnableCommand;

    select();

    const bool commandSent =
        transmit(&command, 1U);

    deselect();

    if (!commandSent)
    {
        return false;
    }

    std::uint8_t status = 0U;

    if (!readStatusRegister1(status))
    {
        return false;
    }

    if ((status & WriteEnableLatchMask) == 0U)
    {
        ++failureCount_;
        return false;
    }

    return true;
}

bool W25q64::waitUntilReady()
{
    const std::uint32_t startTimeMs =
        HAL_GetTick();

    while (true)
    {
        std::uint8_t status = 0U;

        if (!readStatusRegister1(status))
        {
            return false;
        }

        if ((status & BusyMask) == 0U)
        {
            return true;
        }

        if ((HAL_GetTick() - startTimeMs) >=
            OperationTimeoutMs)
        {
            ++failureCount_;
            return false;
        }

        HAL_Delay(1U);
    }
}

bool W25q64::transmit(
    const std::uint8_t* data,
    std::size_t length)
{
    if ((data == nullptr) ||
        (spi_ == nullptr))
    {
        ++failureCount_;
        return false;
    }

    std::size_t offset = 0U;

    while (offset < length)
    {
        const std::size_t remaining =
            length - offset;

        const std::uint16_t chunkLength =
            static_cast<std::uint16_t>(
                (remaining > 65535U)
                    ? 65535U
                    : remaining);

        const HAL_StatusTypeDef status =
            HAL_SPI_Transmit(
                spi_,
                const_cast<std::uint8_t*>(
                    &data[offset]),
                chunkLength,
                CommunicationTimeoutMs);

        if (status != HAL_OK)
        {
            ++failureCount_;
            available_ = false;
            return false;
        }

        offset += chunkLength;
    }

    return true;
}

bool W25q64::receive(
    std::uint8_t* data,
    std::size_t length)
{
    if ((data == nullptr) ||
        (spi_ == nullptr))
    {
        ++failureCount_;
        return false;
    }

    std::size_t offset = 0U;

    while (offset < length)
    {
        const std::size_t remaining =
            length - offset;

        const std::uint16_t chunkLength =
            static_cast<std::uint16_t>(
                (remaining > 65535U)
                    ? 65535U
                    : remaining);

        const HAL_StatusTypeDef status =
            HAL_SPI_Receive(
                spi_,
                &data[offset],
                chunkLength,
                CommunicationTimeoutMs);

        if (status != HAL_OK)
        {
            ++failureCount_;
            available_ = false;
            return false;
        }

        offset += chunkLength;
    }

    return true;
}

bool W25q64::validRange(
    std::uint32_t address,
    std::size_t length) const
{
    if (address >= CapacityBytes)
    {
        return false;
    }

    const std::size_t availableBytes =
        static_cast<std::size_t>(
            CapacityBytes - address);

    return length <= availableBytes;
}

void W25q64::select()
{
    HAL_GPIO_WritePin(
        chipSelectPort_,
        chipSelectPin_,
        GPIO_PIN_RESET);
}

void W25q64::deselect()
{
    HAL_GPIO_WritePin(
        chipSelectPort_,
        chipSelectPin_,
        GPIO_PIN_SET);
}
