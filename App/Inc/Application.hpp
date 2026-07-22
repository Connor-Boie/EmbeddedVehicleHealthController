#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "ButtonDebouncer.hpp"
#include "DigitalOutput.hpp"
#include "PeriodicTimer.hpp"

#include <cstdint>

class Application
{
public:
    Application();

    void initialize();
    void run();

    [[nodiscard]] std::uint32_t buttonPressCount() const;
    [[nodiscard]] std::uint32_t heartbeatExecutionCount() const;
    [[nodiscard]] std::uint32_t healthCheckCount() const;
    [[nodiscard]] bool heartbeatEnabled() const;
    [[nodiscard]] bool systemHealthy() const;

private:
    void processButton(std::uint32_t currentTimeMs);
    void updateHeartbeat();
    void performHealthCheck(std::uint32_t currentTimeMs);

    [[nodiscard]] bool readUserButtonPressed() const;

    DigitalOutput statusLed_;
    ButtonDebouncer buttonDebouncer_;

    PeriodicTimer buttonSampleTimer_;
    PeriodicTimer heartbeatTimer_;
    PeriodicTimer healthCheckTimer_;

    std::uint32_t buttonPressCount_{0U};
    std::uint32_t heartbeatExecutionCount_{0U};
    std::uint32_t healthCheckCount_{0U};

    std::uint32_t lastButtonTaskTimeMs_{0U};

    bool heartbeatEnabled_{true};
    bool systemHealthy_{true};
};

#endif
