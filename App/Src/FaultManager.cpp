#include "FaultManager.hpp"

void FaultManager::setFault(Fault fault, bool active)
{
    const std::uint32_t faultMask = toMask(fault);

    if (active)
    {
        activeFaultMask_ |= faultMask;
        latchedFaultMask_ |= faultMask;
    }
    else
    {
        activeFaultMask_ &= ~faultMask;
    }
}

void FaultManager::clearLatchedFaults()
{
    latchedFaultMask_ = activeFaultMask_;
}

bool FaultManager::hasActiveFaults() const
{
    return activeFaultMask_ != 0U;
}

std::uint32_t FaultManager::activeFaultMask() const
{
    return activeFaultMask_;
}

std::uint32_t FaultManager::latchedFaultMask() const
{
    return latchedFaultMask_;
}

std::uint32_t FaultManager::toMask(Fault fault)
{
    return static_cast<std::uint32_t>(fault);
}
