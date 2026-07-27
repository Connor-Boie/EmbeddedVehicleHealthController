#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "ButtonDebouncer.hpp"
#include "DigitalOutput.hpp"
#include "PeriodicTimer.hpp"
#include "UartCommandReceiver.hpp"
#include "UartTelemetry.hpp"

#include <cstdint>

class Application
{
public:
    Application();

    void initialize();
    void run();

    void onTimerInterrupt();
    void onUartByteReceived(std::uint8_t byte);
    void onUartReceiveError();

    [[nodiscard]] std::uint32_t buttonPressCount() const;
    [[nodiscard]] std::uint32_t heartbeatExecutionCount() const;
    [[nodiscard]] std::uint32_t healthCheckCount() const;

    [[nodiscard]] std::uint32_t timerInterruptCount() const;
    [[nodiscard]] std::uint32_t processedTimerEventCount() const;

    [[nodiscard]] std::uint32_t telemetryMessageCount() const;
    [[nodiscard]] std::uint32_t telemetryFailureCount() const;

    [[nodiscard]] std::uint32_t receivedLineCount() const;

    [[nodiscard]] bool heartbeatEnabled() const;
    [[nodiscard]] bool systemHealthy() const;
    [[nodiscard]] bool hardwareTimerActive() const;

private:
    void processButton(std::uint32_t currentTimeMs);
    void processUartReceive();
    void updateHeartbeat();
    void performHealthCheck(std::uint32_t currentTimeMs);
    void processTimerEvents();
    void sendTelemetry(std::uint32_t currentTimeMs);

    [[nodiscard]] bool readUserButtonPressed() const;

    DigitalOutput statusLed_;
    ButtonDebouncer buttonDebouncer_;
    UartCommandReceiver uartReceiver_;
    UartTelemetry telemetry_;

    PeriodicTimer buttonSampleTimer_;
    PeriodicTimer heartbeatTimer_;
    PeriodicTimer healthCheckTimer_;
    PeriodicTimer telemetryTimer_;

    char receivedLine_[UartCommandReceiver::LineCapacity]{};

    std::uint32_t receivedLineCount_{0U};
    std::uint32_t buttonPressCount_{0U};
    std::uint32_t heartbeatExecutionCount_{0U};
    std::uint32_t healthCheckCount_{0U};

    volatile std::uint32_t timerInterruptCount_{0U};
    std::uint32_t processedTimerEventCount_{0U};
    std::uint32_t previousHealthCheckTimerCount_{0U};

    std::uint32_t lastButtonTaskTimeMs_{0U};

    bool heartbeatEnabled_{true};
    bool systemHealthy_{true};
    bool hardwareTimerActive_{false};
};

#endif
