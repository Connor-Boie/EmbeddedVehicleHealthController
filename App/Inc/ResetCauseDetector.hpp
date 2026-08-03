#ifndef RESET_CAUSE_DETECTOR_HPP
#define RESET_CAUSE_DETECTOR_HPP

#include <cstdint>

enum class ResetCause
{
    Unknown,
    PowerOn,
    Brownout,
    ExternalPin,
    Software,
    IndependentWatchdog,
    WindowWatchdog,
    LowPower
};

class ResetCauseDetector
{
public:
    void capture();

    [[nodiscard]] ResetCause primaryCause() const;
    [[nodiscard]] std::uint32_t causeMask() const;
    [[nodiscard]] const char* primaryCauseName() const;

private:
    enum CauseBit : std::uint32_t
    {
        PowerOnBit = 1UL << 0U,
        BrownoutBit = 1UL << 1U,
        ExternalPinBit = 1UL << 2U,
        SoftwareBit = 1UL << 3U,
        IndependentWatchdogBit = 1UL << 4U,
        WindowWatchdogBit = 1UL << 5U,
        LowPowerBit = 1UL << 6U
    };

    [[nodiscard]] static ResetCause selectPrimaryCause(
        std::uint32_t causeMask);

    [[nodiscard]] static const char* causeName(
        ResetCause cause);

    ResetCause primaryCause_{ResetCause::Unknown};
    std::uint32_t causeMask_{0U};
};

#endif
