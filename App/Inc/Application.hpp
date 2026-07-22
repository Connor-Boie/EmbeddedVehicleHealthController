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

private:
    void processButton(std::uint32_t currentTimeMs);

    [[nodiscard]] bool readUserButtonPressed() const;

    DigitalOutput statusLed_;
    ButtonDebouncer buttonDebouncer_;
    PeriodicTimer buttonSampleTimer_;

    std::uint32_t buttonPressCount_{0U};
};

#endif
