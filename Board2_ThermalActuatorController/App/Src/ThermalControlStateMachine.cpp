#include "ThermalControlStateMachine.hpp"

void ThermalControlStateMachine::reset()
{
    state_ = ThermalControlState::Safe;
}

void ThermalControlStateMachine::update(
    const RemoteVehicleStatus&
        remoteStatus)
{
    if (remoteStatus.communicationState() !=
        RemoteCommunicationState::
            Connected)
    {
        state_ =
            ThermalControlState::Safe;

        return;
    }

    if (!remoteStatus.temperatureValid())
    {
        state_ =
            ThermalControlState::Safe;

        return;
    }

    const std::int16_t
        temperatureDeciCelsius =
            remoteStatus.
                temperatureDeciCelsius();

    if (temperatureDeciCelsius >=
        CriticalThresholdDeciCelsius)
    {
        state_ =
            ThermalControlState::
                Critical;
    }
    else if (temperatureDeciCelsius >=
             HighThresholdDeciCelsius)
    {
        state_ =
            ThermalControlState::
                High;
    }
    else if (temperatureDeciCelsius >=
             CoolingThresholdDeciCelsius)
    {
        state_ =
            ThermalControlState::
                Cooling;
    }
    else if (temperatureDeciCelsius >=
             WarmThresholdDeciCelsius)
    {
        state_ =
            ThermalControlState::
                Warm;
    }
    else
    {
        state_ =
            ThermalControlState::
                Normal;
    }
}

ThermalControlState
ThermalControlStateMachine::state() const
{
    return state_;
}
