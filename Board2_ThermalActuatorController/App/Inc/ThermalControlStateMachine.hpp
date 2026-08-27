#ifndef THERMAL_CONTROL_STATE_MACHINE_HPP
#define THERMAL_CONTROL_STATE_MACHINE_HPP

#include "RemoteVehicleStatus.hpp"

#include <cstdint>

enum class ThermalControlState
{
    Normal,
    Warm,
    Cooling,
    High,
    Critical,
    Safe
};

class ThermalControlStateMachine
{
public:
    static constexpr std::int16_t
        WarmThresholdDeciCelsius = 350;

    static constexpr std::int16_t
        CoolingThresholdDeciCelsius = 450;

    static constexpr std::int16_t
        HighThresholdDeciCelsius = 550;

    static constexpr std::int16_t
        CriticalThresholdDeciCelsius = 600;

    ThermalControlStateMachine() = default;

    void reset();

    void update(
        const RemoteVehicleStatus&
            remoteStatus);

    [[nodiscard]] ThermalControlState
        state() const;

private:
    ThermalControlState
        state_{ThermalControlState::Safe};
};

#endif
