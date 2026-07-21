#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "DigitalOutput.hpp"

#include <cstdint>

class Application
{
public:
	Application();

    void initialize();
    void run();

    [[nodiscard]] std::uint32_t blinkCount() const;

private:
    DigitalOutput statusLed_;
    std::uint32_t blinkCount_{0U};
};

#endif
