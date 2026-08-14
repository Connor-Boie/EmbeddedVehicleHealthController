#ifndef UART_TELEMETRY_HPP
#define UART_TELEMETRY_HPP

#include "stm32f4xx_hal.h"

#include <cstddef>
#include <cstdint>

class UartTelemetry
{
public:
    explicit UartTelemetry(
        UART_HandleTypeDef* uart);

    [[nodiscard]] bool sendStatus(
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
        std::uint32_t receiveErrorCount);

    [[nodiscard]] bool sendText(
        const char* text);

    [[nodiscard]] std::uint32_t
        messageCount() const;

    [[nodiscard]] std::uint32_t
        failureCount() const;

private:
    static constexpr std::size_t BufferSize =
        1024U;

    static constexpr std::uint32_t
        TransmitTimeoutMs = 150U;

    UART_HandleTypeDef* uart_;

    char buffer_[BufferSize]{};

    std::uint32_t messageCount_{0U};
    std::uint32_t failureCount_{0U};
};

#endif
