#ifndef FAULT_INJECTOR_HPP
#define FAULT_INJECTOR_HPP

#include "FaultManager.hpp"

#include <cstdint>

class FaultInjector
{
public:
    void injectFault(Fault fault);
    void clearAll();

    [[nodiscard]] bool isInjected(Fault fault) const;
    [[nodiscard]] std::uint32_t injectedFaultMask() const;

private:
    [[nodiscard]] static std::uint32_t toMask(Fault fault);

    std::uint32_t injectedFaultMask_{0U};
};

#endif
