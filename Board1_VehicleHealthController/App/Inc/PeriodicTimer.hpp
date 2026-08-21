#ifndef PERIODIC_TIMER_HPP
#define PERIODIC_TIMER_HPP

#include <cstdint>

class PeriodicTimer
{
public:
    explicit PeriodicTimer(std::uint32_t periodMs);

    void initialize(std::uint32_t currentTimeMs);

    [[nodiscard]] bool isDue(std::uint32_t currentTimeMs);

private:
    std::uint32_t periodMs_;
    std::uint32_t previousExecutionTimeMs_{0U};
};

#endif
