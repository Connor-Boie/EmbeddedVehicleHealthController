#ifndef CAN_BUS_HPP
#define CAN_BUS_HPP

#include "stm32f4xx_hal.h"

#include <cstdint>

struct CanFrame
{
    std::uint32_t id{0U};
    std::uint8_t length{0U};
    std::uint8_t data[8]{};
};

class CanBus
{
public:
    explicit CanBus(
        CAN_HandleTypeDef* can);

    [[nodiscard]] bool initialize();

    [[nodiscard]] bool send(
        const CanFrame& frame);

    [[nodiscard]] bool receive(
        CanFrame& frame);

    [[nodiscard]] bool initialized() const;

    [[nodiscard]] bool messagePending() const;

    [[nodiscard]] std::uint32_t
        transmitCount() const;

    [[nodiscard]] std::uint32_t
        transmitFailureCount() const;

    [[nodiscard]] std::uint32_t
        receiveCount() const;

    [[nodiscard]] std::uint32_t
        receiveFailureCount() const;

private:
    static constexpr std::uint32_t
        MaximumStandardId = 0x7FFU;

    static constexpr std::uint8_t
        MaximumPayloadLength = 8U;

    CAN_HandleTypeDef* can_;

    std::uint32_t transmitCount_{0U};
    std::uint32_t transmitFailureCount_{0U};
    std::uint32_t receiveCount_{0U};
    std::uint32_t receiveFailureCount_{0U};

    bool initialized_{false};
};

#endif
