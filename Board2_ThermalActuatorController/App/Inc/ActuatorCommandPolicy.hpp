#ifndef ACTUATOR_COMMAND_POLICY_HPP
#define ACTUATOR_COMMAND_POLICY_HPP

#include "ThermalControlStateMachine.hpp"

#include <cstdint>

enum class WarningColor
{
    Green,
    Yellow,
    Blue,
    Orange,
    Red,
    Magenta
};

enum class BuzzerPattern
{
    Off,
    SlowBeep,
    FastBeep,
    Fault
};

struct ActuatorCommand
{
    std::uint8_t
        coolingDutyPercent{100U};

    WarningColor
        warningColor{
            WarningColor::Magenta};

    BuzzerPattern
        buzzerPattern{
            BuzzerPattern::Fault};
};

class ActuatorCommandPolicy
{
public:
    static constexpr std::uint8_t
        CoolingDutyPercent = 40U;

    static constexpr std::uint8_t
        HighDutyPercent = 70U;

    static constexpr std::uint8_t
        FullDutyPercent = 100U;

    ActuatorCommandPolicy() = default;

    void reset();

    void update(
        ThermalControlState
            thermalState);

    [[nodiscard]] const ActuatorCommand&
        command() const;

private:
    ActuatorCommand command_{};
};

#endif
