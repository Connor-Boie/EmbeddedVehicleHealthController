#ifndef FAULT_MANAGER_HPP
#define FAULT_MANAGER_HPP

#include <cstdint>

enum class Fault : std::uint32_t
{
    ButtonTaskTimeout = 1UL << 0U,
    HardwareTimerInactive = 1UL << 1U
};

class FaultManager
{
public:
    void setFault(Fault fault, bool active);
    void clearLatchedFaults();

    [[nodiscard]] bool hasActiveFaults() const;

    [[nodiscard]] std::uint32_t activeFaultMask() const;
    [[nodiscard]] std::uint32_t latchedFaultMask() const;

private:
    [[nodiscard]] static std::uint32_t toMask(Fault fault);

    std::uint32_t activeFaultMask_{0U};
    std::uint32_t latchedFaultMask_{0U};
};

#endif
