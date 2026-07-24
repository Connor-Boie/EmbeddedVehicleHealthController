#ifndef UART_TELEMETRY_HPP
#define UART_TELEMETRY_HPP

#include "stm32f4xx_hal.h"

#include <cstddef>
#include <cstdint>

class UartTelemetry
{
public:
    explicit UartTelemetry(UART_HandleTypeDef* uart);

    [[nodiscard]] bool sendStatus(
        std::uint32_t uptimeMs,
        std::uint32_t buttonPressCount,
        bool heartbeatEnabled,
        bool systemHealthy,
        bool hardwareTimerActive,
        std::uint32_t timerInterruptCount);

    [[nodiscard]] std::uint32_t messageCount() const;
    [[nodiscard]] std::uint32_t failureCount() const;

private:
    static constexpr std::size_t BufferSize = 192U;
    static constexpr std::uint32_t TransmitTimeoutMs = 50U;

    UART_HandleTypeDef* uart_;

    char buffer_[BufferSize]{};

    std::uint32_t messageCount_{0U};
    std::uint32_t failureCount_{0U};
};

#endif
