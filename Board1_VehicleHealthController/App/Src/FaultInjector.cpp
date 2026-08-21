#include "FaultInjector.hpp"

void FaultInjector::injectFault(Fault fault)
{
    injectedFaultMask_ |= toMask(fault);
}

void FaultInjector::clearAll()
{
    injectedFaultMask_ = 0U;
}

bool FaultInjector::isInjected(Fault fault) const
{
    return (injectedFaultMask_ & toMask(fault)) != 0U;
}

std::uint32_t FaultInjector::injectedFaultMask() const
{
    return injectedFaultMask_;
}

std::uint32_t FaultInjector::toMask(Fault fault)
{
    return static_cast<std::uint32_t>(fault);
}
