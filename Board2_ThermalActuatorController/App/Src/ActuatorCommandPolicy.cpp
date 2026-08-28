#include "ActuatorCommandPolicy.hpp"

void ActuatorCommandPolicy::reset()
{
    command_.coolingDutyPercent =
        FullDutyPercent;

    command_.warningColor =
        WarningColor::Magenta;

    command_.buzzerPattern =
        BuzzerPattern::Fault;
}

void ActuatorCommandPolicy::update(
    ThermalControlState thermalState)
{
    switch (thermalState)
    {
        case ThermalControlState::Normal:
        {
            command_.coolingDutyPercent =
                0U;

            command_.warningColor =
                WarningColor::Green;

            command_.buzzerPattern =
                BuzzerPattern::Off;

            break;
        }

        case ThermalControlState::Warm:
        {
            command_.coolingDutyPercent =
                0U;

            command_.warningColor =
                WarningColor::Yellow;

            command_.buzzerPattern =
                BuzzerPattern::Off;

            break;
        }

        case ThermalControlState::Cooling:
        {
            command_.coolingDutyPercent =
                CoolingDutyPercent;

            command_.warningColor =
                WarningColor::Blue;

            command_.buzzerPattern =
                BuzzerPattern::Off;

            break;
        }

        case ThermalControlState::High:
        {
            command_.coolingDutyPercent =
                HighDutyPercent;

            command_.warningColor =
                WarningColor::Orange;

            command_.buzzerPattern =
                BuzzerPattern::SlowBeep;

            break;
        }

        case ThermalControlState::Critical:
        {
            command_.coolingDutyPercent =
                FullDutyPercent;

            command_.warningColor =
                WarningColor::Red;

            command_.buzzerPattern =
                BuzzerPattern::FastBeep;

            break;
        }

        case ThermalControlState::Safe:
        {
            command_.coolingDutyPercent =
                FullDutyPercent;

            command_.warningColor =
                WarningColor::Magenta;

            command_.buzzerPattern =
                BuzzerPattern::Fault;

            break;
        }
    }
}

const ActuatorCommand&
ActuatorCommandPolicy::command() const
{
    return command_;
}
