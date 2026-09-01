#ifndef BUZZER_PATTERN_SEQUENCER_HPP
#define BUZZER_PATTERN_SEQUENCER_HPP

#include "ActuatorCommandPolicy.hpp"

#include <cstdint>

class BuzzerPatternSequencer
{
public:
    static constexpr std::uint32_t
        SlowBeepOnTimeMs = 250U;

    static constexpr std::uint32_t
        SlowBeepPeriodMs = 1000U;

    static constexpr std::uint32_t
        FastBeepOnTimeMs = 200U;

    static constexpr std::uint32_t
        FastBeepPeriodMs = 400U;

    static constexpr std::uint32_t
        FaultFirstBeepEndMs = 150U;

    static constexpr std::uint32_t
        FaultSecondBeepStartMs = 300U;

    static constexpr std::uint32_t
        FaultSecondBeepEndMs = 450U;

    static constexpr std::uint32_t
        FaultPeriodMs = 1500U;

    BuzzerPatternSequencer() = default;

    void reset(
        std::uint32_t currentTimeMs);

    void update(
        BuzzerPattern requestedPattern,
        std::uint32_t currentTimeMs);

    [[nodiscard]] BuzzerPattern
        activePattern() const;

    [[nodiscard]] bool
        outputActive() const;

private:
    BuzzerPattern
        activePattern_{
            BuzzerPattern::Off};

    std::uint32_t
        patternStartTimeMs_{0U};

    bool outputActive_{false};
};

#endif
