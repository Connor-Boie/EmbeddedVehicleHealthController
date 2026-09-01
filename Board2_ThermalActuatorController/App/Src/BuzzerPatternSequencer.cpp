#include "BuzzerPatternSequencer.hpp"

void BuzzerPatternSequencer::reset(
    std::uint32_t currentTimeMs)
{
    activePattern_ =
        BuzzerPattern::Off;

    patternStartTimeMs_ =
        currentTimeMs;

    outputActive_ = false;
}

void BuzzerPatternSequencer::update(
    BuzzerPattern requestedPattern,
    std::uint32_t currentTimeMs)
{
    if (requestedPattern !=
        activePattern_)
    {
        activePattern_ =
            requestedPattern;

        patternStartTimeMs_ =
            currentTimeMs;
    }

    const std::uint32_t
        elapsedTimeMs =
            currentTimeMs -
            patternStartTimeMs_;

    switch (activePattern_)
    {
        case BuzzerPattern::Off:
        {
            outputActive_ = false;
            break;
        }

        case BuzzerPattern::SlowBeep:
        {
            const std::uint32_t
                cycleTimeMs =
                    elapsedTimeMs %
                    SlowBeepPeriodMs;

            outputActive_ =
                cycleTimeMs <
                SlowBeepOnTimeMs;

            break;
        }

        case BuzzerPattern::FastBeep:
        {
            const std::uint32_t
                cycleTimeMs =
                    elapsedTimeMs %
                    FastBeepPeriodMs;

            outputActive_ =
                cycleTimeMs <
                FastBeepOnTimeMs;

            break;
        }

        case BuzzerPattern::Fault:
        {
            const std::uint32_t
                cycleTimeMs =
                    elapsedTimeMs %
                    FaultPeriodMs;

            const bool
                firstBeepActive =
                    cycleTimeMs <
                    FaultFirstBeepEndMs;

            const bool
                secondBeepActive =
                    (cycleTimeMs >=
                     FaultSecondBeepStartMs) &&
                    (cycleTimeMs <
                     FaultSecondBeepEndMs);

            outputActive_ =
                firstBeepActive ||
                secondBeepActive;

            break;
        }
    }
}

BuzzerPattern
BuzzerPatternSequencer::activePattern() const
{
    return activePattern_;
}

bool BuzzerPatternSequencer::
    outputActive() const
{
    return outputActive_;
}
