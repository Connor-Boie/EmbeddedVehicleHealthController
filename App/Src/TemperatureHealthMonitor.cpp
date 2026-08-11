#include "TemperatureHealthMonitor.hpp"

void TemperatureHealthMonitor::update(
    bool sensorAAvailable,
    std::int32_t sensorATemperatureMilliCelsius,
    bool sensorBAvailable,
    std::int32_t sensorBTemperatureMilliCelsius)
{
    sensorAFaultActive_ = !sensorAAvailable;
    sensorBFaultActive_ = !sensorBAvailable;

    disagreementFaultActive_ = false;
    overtemperatureFaultActive_ = false;

    disagreementMilliCelsius_ = 0U;

    selectedTemperatureValid_ = false;
    selectedTemperatureMilliCelsius_ = 0;

    if (sensorAAvailable && sensorBAvailable)
    {
        disagreementMilliCelsius_ =
            absoluteDifference(
                sensorATemperatureMilliCelsius,
                sensorBTemperatureMilliCelsius);

        disagreementFaultActive_ =
            disagreementMilliCelsius_ >
            DisagreementThresholdMilliCelsius;

        if (disagreementFaultActive_)
        {
            mode_ = TemperatureMode::Disagreement;
        }
        else
        {
            mode_ = TemperatureMode::Redundant;

            selectedTemperatureMilliCelsius_ =
                sensorATemperatureMilliCelsius +
                ((sensorBTemperatureMilliCelsius -
                  sensorATemperatureMilliCelsius) / 2);

            selectedTemperatureValid_ = true;
        }
    }
    else if (sensorAAvailable)
    {
        mode_ = TemperatureMode::DegradedSensorA;

        selectedTemperatureMilliCelsius_ =
            sensorATemperatureMilliCelsius;

        selectedTemperatureValid_ = true;
    }
    else if (sensorBAvailable)
    {
        mode_ = TemperatureMode::DegradedSensorB;

        selectedTemperatureMilliCelsius_ =
            sensorBTemperatureMilliCelsius;

        selectedTemperatureValid_ = true;
    }
    else
    {
        mode_ = TemperatureMode::Unavailable;
    }

    if (sensorAAvailable &&
        (sensorATemperatureMilliCelsius >=
         OvertemperatureThresholdMilliCelsius))
    {
        overtemperatureFaultActive_ = true;
    }

    if (sensorBAvailable &&
        (sensorBTemperatureMilliCelsius >=
         OvertemperatureThresholdMilliCelsius))
    {
        overtemperatureFaultActive_ = true;
    }
}

TemperatureMode TemperatureHealthMonitor::mode() const
{
    return mode_;
}

const char* TemperatureHealthMonitor::modeName() const
{
    return temperatureModeName(mode_);
}

bool TemperatureHealthMonitor::
selectedTemperatureValid() const
{
    return selectedTemperatureValid_;
}

std::int32_t TemperatureHealthMonitor::
selectedTemperatureMilliCelsius() const
{
    return selectedTemperatureMilliCelsius_;
}

std::uint32_t TemperatureHealthMonitor::
disagreementMilliCelsius() const
{
    return disagreementMilliCelsius_;
}

bool TemperatureHealthMonitor::
sensorAFaultActive() const
{
    return sensorAFaultActive_;
}

bool TemperatureHealthMonitor::
sensorBFaultActive() const
{
    return sensorBFaultActive_;
}

bool TemperatureHealthMonitor::
disagreementFaultActive() const
{
    return disagreementFaultActive_;
}

bool TemperatureHealthMonitor::
overtemperatureFaultActive() const
{
    return overtemperatureFaultActive_;
}

std::uint32_t
TemperatureHealthMonitor::absoluteDifference(
    std::int32_t first,
    std::int32_t second)
{
    if (first >= second)
    {
        return static_cast<std::uint32_t>(
            first - second);
    }

    return static_cast<std::uint32_t>(
        second - first);
}

const char*
TemperatureHealthMonitor::temperatureModeName(
    TemperatureMode mode)
{
    switch (mode)
    {
        case TemperatureMode::Redundant:
            return "REDUNDANT";

        case TemperatureMode::DegradedSensorA:
            return "DEGRADED_A";

        case TemperatureMode::DegradedSensorB:
            return "DEGRADED_B";

        case TemperatureMode::Disagreement:
            return "DISAGREEMENT";

        case TemperatureMode::Unavailable:
        default:
            return "UNAVAILABLE";
    }
}
