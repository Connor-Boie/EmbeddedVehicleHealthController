#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "ButtonDebouncer.hpp"
#include "CommandParser.hpp"
#include "DigitalOutput.hpp"
#include "FaultInjector.hpp"
#include "FaultManager.hpp"
#include "PeriodicTimer.hpp"
#include "UartCommandReceiver.hpp"
#include "UartTelemetry.hpp"
#include "Watchdog.hpp"

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
    [[nodiscard]] std::uint32_t validCommandCount() const;
    [[nodiscard]] std::uint32_t invalidCommandCount() const;

    [[nodiscard]] std::uint32_t activeFaultMask() const;
    [[nodiscard]] std::uint32_t latchedFaultMask() const;
    [[nodiscard]] std::uint32_t injectedFaultMask() const;

    [[nodiscard]] std::uint32_t watchdogRefreshCount() const;
    [[nodiscard]] std::uint32_t watchdogFailureCount() const;

    [[nodiscard]] bool heartbeatEnabled() const;
    [[nodiscard]] bool systemHealthy() const;
    [[nodiscard]] bool hardwareTimerActive() const;
    [[nodiscard]] bool watchdogRefreshEnabled() const;

private:
    void processButton(std::uint32_t currentTimeMs);
    void processUartReceive(std::uint32_t currentTimeMs);
    void handleCommand(
        CommandType command,
        std::uint32_t currentTimeMs);
    void clearApplicationCounters();

    void updateHeartbeat();
    void performHealthCheck(std::uint32_t currentTimeMs);
    void processTimerEvents();
    void refreshWatchdog();
    void sendTelemetry(std::uint32_t currentTimeMs);

    [[nodiscard]] bool readUserButtonPressed() const;

    DigitalOutput statusLed_;
    ButtonDebouncer buttonDebouncer_;
    FaultInjector faultInjector_;
    FaultManager faultManager_;
    UartCommandReceiver uartReceiver_;
    UartTelemetry telemetry_;
    Watchdog watchdog_;

    PeriodicTimer buttonSampleTimer_;
    PeriodicTimer heartbeatTimer_;
    PeriodicTimer healthCheckTimer_;
    PeriodicTimer telemetryTimer_;
    PeriodicTimer watchdogRefreshTimer_;

    char receivedLine_[UartCommandReceiver::LineCapacity]{};

    std::uint32_t receivedLineCount_{0U};
    std::uint32_t validCommandCount_{0U};
    std::uint32_t invalidCommandCount_{0U};

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
    bool watchdogRefreshEnabled_{true};
};

#endif
