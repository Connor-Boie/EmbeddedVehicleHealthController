#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "CanBus.hpp"

#include <cstdint>

class Application
{
public:
    Application();

    void initialize();
    void run();

private:
    void processCanReceive();

    void processVehicleHealthStatus(
        const CanFrame& frame);

    void sendStartupMessage(
        bool canInitialized);

    void sendVehicleHealthStatus(
        std::uint8_t protocolVersion,
        bool systemHealthy,
        bool temperatureValid,
        bool sensorAAvailable,
        bool sensorBAvailable,
        std::int16_t temperatureDeciCelsius,
        std::uint32_t faultMask);

    void sendText(
        const char* text);

    CanBus canBus_;

    std::uint32_t vehicleHealthReceiveCount_{0U};
    std::uint32_t invalidVehicleHealthCount_{0U};
};

#endif
