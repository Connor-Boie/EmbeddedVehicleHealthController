#include "UartTelemetry.hpp"

#include <cstdio>

UartTelemetry::UartTelemetry(UART_HandleTypeDef* uart)
    : uart_{uart}
{
}

bool UartTelemetry::sendStatus(
    std::uint32_t uptimeMs,
    std::uint32_t buttonPressCount,
    bool heartbeatEnabled,
    bool systemHealthy,
    bool hardwareTimerActive,
    std::uint32_t timerInterruptCount,
    std::uint32_t receivedLineCount,
    std::uint32_t droppedByteCount,
    std::uint32_t overflowLineCount,
    std::uint32_t receiveErrorCount)
{
    const int formattedLength = std::snprintf(
        buffer_,
        BufferSize,
        "uptime_ms=%lu "
        "button_presses=%lu "
        "heartbeat_enabled=%u "
        "healthy=%u "
        "timer_active=%u "
        "timer_irq_count=%lu "
        "rx_lines=%lu "
        "rx_dropped_bytes=%lu "
        "rx_overflow_lines=%lu "
        "rx_errors=%lu\r\n",
        static_cast<unsigned long>(uptimeMs),
        static_cast<unsigned long>(buttonPressCount),
        heartbeatEnabled ? 1U : 0U,
        systemHealthy ? 1U : 0U,
        hardwareTimerActive ? 1U : 0U,
        static_cast<unsigned long>(timerInterruptCount),
        static_cast<unsigned long>(receivedLineCount),
        static_cast<unsigned long>(droppedByteCount),
        static_cast<unsigned long>(overflowLineCount),
        static_cast<unsigned long>(receiveErrorCount));

    if ((formattedLength < 0) ||
        (static_cast<std::size_t>(formattedLength) >= BufferSize))
    {
        ++failureCount_;
        return false;
    }

    const HAL_StatusTypeDef transmitStatus = HAL_UART_Transmit(
        uart_,
        reinterpret_cast<std::uint8_t*>(buffer_),
        static_cast<std::uint16_t>(formattedLength),
        TransmitTimeoutMs);

    if (transmitStatus != HAL_OK)
    {
        ++failureCount_;
        return false;
    }

    ++messageCount_;
    return true;
}

std::uint32_t UartTelemetry::messageCount() const
{
    return messageCount_;
}

std::uint32_t UartTelemetry::failureCount() const
{
    return failureCount_;
}
