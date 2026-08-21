#ifndef TEMPERATURE_HEALTH_MONITOR_HPP
#define TEMPERATURE_HEALTH_MONITOR_HPP

#include <cstdint>

enum class TemperatureMode
{
    Redundant,
    DegradedSensorA,
    DegradedSensorB,
    Disagreement,
    Unavailable
};

class TemperatureHealthMonitor
{
public:
    void update(
        bool sensorAAvailable,
        std::int32_t sensorATemperatureMilliCelsius,
        bool sensorBAvailable,
        std::int32_t sensorBTemperatureMilliCelsius);

    [[nodiscard]] TemperatureMode mode() const;
    [[nodiscard]] const char* modeName() const;

    [[nodiscard]] bool selectedTemperatureValid() const;

    [[nodiscard]] std::int32_t
        selectedTemperatureMilliCelsius() const;

    [[nodiscard]] std::uint32_t
        disagreementMilliCelsius() const;

    [[nodiscard]] bool sensorAFaultActive() const;
    [[nodiscard]] bool sensorBFaultActive() const;
    [[nodiscard]] bool disagreementFaultActive() const;
    [[nodiscard]] bool overtemperatureFaultActive() const;

private:
    [[nodiscard]] static std::uint32_t
        absoluteDifference(
            std::int32_t first,
            std::int32_t second);

    [[nodiscard]] static const char*
        temperatureModeName(
            TemperatureMode mode);

    static constexpr std::uint32_t
        DisagreementThresholdMilliCelsius = 2000U;

    static constexpr std::int32_t
        OvertemperatureThresholdMilliCelsius = 60000;

    TemperatureMode mode_{
        TemperatureMode::Unavailable};

    std::int32_t selectedTemperatureMilliCelsius_{0};

    std::uint32_t disagreementMilliCelsius_{0U};

    bool selectedTemperatureValid_{false};

    bool sensorAFaultActive_{true};
    bool sensorBFaultActive_{true};
    bool disagreementFaultActive_{false};
    bool overtemperatureFaultActive_{false};
};

#endif
