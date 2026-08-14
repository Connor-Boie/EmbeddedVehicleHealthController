#include "UartTelemetry.hpp"

#include <cstdio>

UartTelemetry::UartTelemetry(
    UART_HandleTypeDef* uart)
    : uart_{uart}
{
}

bool UartTelemetry::sendStatus(
    std::uint32_t uptimeMs,
    const char* resetCauseName,
    std::uint32_t resetCauseMask,

    bool sensorAAvailable,
    std::int32_t sensorATemperatureMilliCelsius,
    std::uint32_t sensorAReadCount,
    std::uint32_t sensorAFailureCount,

    bool sensorBAvailable,
    std::int32_t sensorBTemperatureMilliCelsius,
    std::uint32_t sensorBReadCount,
    std::uint32_t sensorBFailureCount,

    const char* temperatureModeName,
    bool selectedTemperatureValid,
    std::int32_t selectedTemperatureMilliCelsius,
    std::uint32_t temperatureDisagreementMilliCelsius,

    bool flashAvailable,
    std::uint32_t flashJedecId,
    std::uint32_t flashFailureCount,
    bool flashTestRun,
    bool flashTestPassed,

    std::uint32_t buttonPressCount,
    std::uint32_t heartbeatExecutionCount,

    bool systemHealthy,
    bool hardwareTimerActive,
    std::uint32_t timerInterruptCount,

    std::uint32_t receivedLineCount,
    std::uint32_t validCommandCount,
    std::uint32_t invalidCommandCount,

    std::uint32_t activeFaultMask,
    std::uint32_t latchedFaultMask,
    std::uint32_t injectedFaultMask,

    bool watchdogRefreshEnabled,
    std::uint32_t watchdogRefreshCount,
    std::uint32_t watchdogFailureCount,

    std::uint32_t droppedByteCount,
    std::uint32_t overflowLineCount,
    std::uint32_t receiveErrorCount)
{
    if ((resetCauseName == nullptr) ||
        (temperatureModeName == nullptr))
    {
        ++failureCount_;
        return false;
    }

    const int formattedLength =
        std::snprintf(
            buffer_,
            BufferSize,

            "uptime_ms=%lu "
            "reset_cause=%s "
            "reset_cause_mask=0x%08lX "

            "temp_a_available=%u "
            "temp_a_mC=%ld "
            "temp_a_reads=%lu "
            "temp_a_failures=%lu "

            "temp_b_available=%u "
            "temp_b_mC=%ld "
            "temp_b_reads=%lu "
            "temp_b_failures=%lu "

            "temp_mode=%s "
            "temp_selected_valid=%u "
            "temp_selected_mC=%ld "
            "temp_disagreement_mC=%lu "

            "flash_available=%u "
            "flash_jedec_id=0x%06lX "
            "flash_failures=%lu "
            "flash_test_run=%u "
            "flash_test_passed=%u "

            "button_presses=%lu "
            "heartbeat_count=%lu "

            "healthy=%u "
            "timer_active=%u "
            "timer_irq_count=%lu "

            "rx_lines=%lu "
            "valid_commands=%lu "
            "invalid_commands=%lu "

            "active_faults=0x%08lX "
            "latched_faults=0x%08lX "
            "injected_faults=0x%08lX "

            "watchdog_refresh_enabled=%u "
            "watchdog_refreshes=%lu "
            "watchdog_failures=%lu "

            "rx_dropped_bytes=%lu "
            "rx_overflow_lines=%lu "
            "rx_errors=%lu\r\n",

            static_cast<unsigned long>(
                uptimeMs),

            resetCauseName,

            static_cast<unsigned long>(
                resetCauseMask),

            sensorAAvailable ? 1U : 0U,

            static_cast<long>(
                sensorATemperatureMilliCelsius),

            static_cast<unsigned long>(
                sensorAReadCount),

            static_cast<unsigned long>(
                sensorAFailureCount),

            sensorBAvailable ? 1U : 0U,

            static_cast<long>(
                sensorBTemperatureMilliCelsius),

            static_cast<unsigned long>(
                sensorBReadCount),

            static_cast<unsigned long>(
                sensorBFailureCount),

            temperatureModeName,

            selectedTemperatureValid ? 1U : 0U,

            static_cast<long>(
                selectedTemperatureMilliCelsius),

            static_cast<unsigned long>(
                temperatureDisagreementMilliCelsius),

            flashAvailable ? 1U : 0U,

            static_cast<unsigned long>(
                flashJedecId),

            static_cast<unsigned long>(
                flashFailureCount),

            flashTestRun ? 1U : 0U,
            flashTestPassed ? 1U : 0U,

            static_cast<unsigned long>(
                buttonPressCount),

            static_cast<unsigned long>(
                heartbeatExecutionCount),

            systemHealthy ? 1U : 0U,
            hardwareTimerActive ? 1U : 0U,

            static_cast<unsigned long>(
                timerInterruptCount),

            static_cast<unsigned long>(
                receivedLineCount),

            static_cast<unsigned long>(
                validCommandCount),

            static_cast<unsigned long>(
                invalidCommandCount),

            static_cast<unsigned long>(
                activeFaultMask),

            static_cast<unsigned long>(
                latchedFaultMask),

            static_cast<unsigned long>(
                injectedFaultMask),

            watchdogRefreshEnabled ? 1U : 0U,

            static_cast<unsigned long>(
                watchdogRefreshCount),

            static_cast<unsigned long>(
                watchdogFailureCount),

            static_cast<unsigned long>(
                droppedByteCount),

            static_cast<unsigned long>(
                overflowLineCount),

            static_cast<unsigned long>(
                receiveErrorCount));

    if ((formattedLength < 0) ||
        (static_cast<std::size_t>(
             formattedLength) >= BufferSize))
    {
        ++failureCount_;
        return false;
    }

    const HAL_StatusTypeDef transmitStatus =
        HAL_UART_Transmit(
            uart_,
            reinterpret_cast<std::uint8_t*>(
                buffer_),
            static_cast<std::uint16_t>(
                formattedLength),
            TransmitTimeoutMs);

    if (transmitStatus != HAL_OK)
    {
        ++failureCount_;
        return false;
    }

    ++messageCount_;
    return true;
}

bool UartTelemetry::sendText(
    const char* text)
{
    if (text == nullptr)
    {
        ++failureCount_;
        return false;
    }

    const int formattedLength =
        std::snprintf(
            buffer_,
            BufferSize,
            "%s\r\n",
            text);

    if ((formattedLength < 0) ||
        (static_cast<std::size_t>(
             formattedLength) >= BufferSize))
    {
        ++failureCount_;
        return false;
    }

    const HAL_StatusTypeDef transmitStatus =
        HAL_UART_Transmit(
            uart_,
            reinterpret_cast<std::uint8_t*>(
                buffer_),
            static_cast<std::uint16_t>(
                formattedLength),
            TransmitTimeoutMs);

    if (transmitStatus != HAL_OK)
    {
        ++failureCount_;
        return false;
    }

    ++messageCount_;
    return true;
}

std::uint32_t
UartTelemetry::messageCount() const
{
    return messageCount_;
}

std::uint32_t
UartTelemetry::failureCount() const
{
    return failureCount_;
}
