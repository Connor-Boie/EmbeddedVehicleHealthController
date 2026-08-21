#ifndef W25Q64_HPP
#define W25Q64_HPP

#include "stm32f4xx_hal.h"

#include <cstddef>
#include <cstdint>

class W25q64
{
public:
    W25q64(
        SPI_HandleTypeDef* spi,
        GPIO_TypeDef* chipSelectPort,
        std::uint16_t chipSelectPin);

    [[nodiscard]] bool initialize();

    [[nodiscard]] bool read(
        std::uint32_t address,
        std::uint8_t* data,
        std::size_t length);

    [[nodiscard]] bool program(
        std::uint32_t address,
        const std::uint8_t* data,
        std::size_t length);

    [[nodiscard]] bool eraseSector(
        std::uint32_t address);

    [[nodiscard]] bool available() const;
    [[nodiscard]] std::uint32_t jedecId() const;
    [[nodiscard]] std::uint32_t failureCount() const;

    static constexpr std::uint32_t CapacityBytes =
        8U * 1024U * 1024U;

    static constexpr std::uint32_t SectorSizeBytes =
        4096U;

    static constexpr std::uint32_t PageSizeBytes =
        256U;

private:
    [[nodiscard]] bool readJedecId();
    [[nodiscard]] bool readStatusRegister1(
        std::uint8_t& status);

    [[nodiscard]] bool writeEnable();
    [[nodiscard]] bool waitUntilReady();

    [[nodiscard]] bool transmit(
        const std::uint8_t* data,
        std::size_t length);

    [[nodiscard]] bool receive(
        std::uint8_t* data,
        std::size_t length);

    [[nodiscard]] bool validRange(
        std::uint32_t address,
        std::size_t length) const;

    void select();
    void deselect();

    static constexpr std::uint8_t ReadDataCommand =
        0x03U;

    static constexpr std::uint8_t PageProgramCommand =
        0x02U;

    static constexpr std::uint8_t WriteEnableCommand =
        0x06U;

    static constexpr std::uint8_t
        ReadStatusRegister1Command = 0x05U;

    static constexpr std::uint8_t SectorEraseCommand =
        0x20U;

    static constexpr std::uint8_t ReadJedecIdCommand =
        0x9FU;

    static constexpr std::uint8_t BusyMask =
        1U << 0U;

    static constexpr std::uint8_t WriteEnableLatchMask =
        1U << 1U;

    static constexpr std::uint8_t
        ExpectedManufacturerId = 0xEFU;

    static constexpr std::uint8_t
        ExpectedCapacityId = 0x17U;

    static constexpr std::uint32_t
        CommunicationTimeoutMs = 100U;

    static constexpr std::uint32_t
        OperationTimeoutMs = 1000U;

    SPI_HandleTypeDef* spi_;
    GPIO_TypeDef* chipSelectPort_;
    std::uint16_t chipSelectPin_;

    std::uint32_t jedecId_{0U};
    std::uint32_t failureCount_{0U};

    bool available_{false};
};

#endif
