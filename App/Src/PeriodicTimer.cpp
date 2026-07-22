#include "PeriodicTimer.hpp"

PeriodicTimer::PeriodicTimer(std::uint32_t periodMs)
    : periodMs_{periodMs}
{
}

void PeriodicTimer::initialize(std::uint32_t currentTimeMs)
{
    previousExecutionTimeMs_ = currentTimeMs;
}

bool PeriodicTimer::isDue(std::uint32_t currentTimeMs)
{
    const std::uint32_t elapsedTimeMs =
        currentTimeMs - previousExecutionTimeMs_;

    if (elapsedTimeMs < periodMs_)
    {
        return false;
    }

    previousExecutionTimeMs_ = currentTimeMs;
    return true;
}
