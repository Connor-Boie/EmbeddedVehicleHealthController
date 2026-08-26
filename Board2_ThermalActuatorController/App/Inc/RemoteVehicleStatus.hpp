#ifndef REMOTE_VEHICLE_STATUS_HPP
#define REMOTE_VEHICLE_STATUS_HPP

#include "CanBus.hpp"

#include <cstdint>

enum class RemoteCommunicationState
{
    WaitingForData,
    Connected,
    CommunicationLost
};

class RemoteVehicleStatus
{
public:
    static constexpr std::uint32_t
        CommunicationTimeoutMs = 1500U;

    RemoteVehicleStatus() = default;

    void reset();

    [[nodiscard]] bool processFrame(
        const CanFrame& frame,
        std::uint32_t currentTimeMs);

    void updateCommunicationState(
        std::uint32_t currentTimeMs);

    [[nodiscard]] RemoteCommunicationState
        communicationState() const;

    [[nodiscard]] bool
        hasReceivedValidFrame() const;

    [[nodiscard]] bool
        systemHealthy() const;

    [[nodiscard]] bool
        temperatureValid() const;

    [[nodiscard]] bool
        sensorAAvailable() const;

    [[nodiscard]] bool
        sensorBAvailable() const;

    [[nodiscard]] std::int16_t
        temperatureDeciCelsius() const;

    [[nodiscard]] std::uint32_t
        faultMask() const;

    [[nodiscard]] std::uint32_t
        lastReceiveTimeMs() const;

    [[nodiscard]] std::uint32_t
        validFrameCount() const;

    [[nodiscard]] std::uint32_t
        invalidFrameCount() const;

private:
    bool hasReceivedValidFrame_{false};

    bool systemHealthy_{false};
    bool temperatureValid_{false};
    bool sensorAAvailable_{false};
    bool sensorBAvailable_{false};

    std::int16_t
        temperatureDeciCelsius_{0};

    std::uint32_t faultMask_{0U};

    std::uint32_t lastReceiveTimeMs_{0U};

    std::uint32_t validFrameCount_{0U};
    std::uint32_t invalidFrameCount_{0U};

    RemoteCommunicationState
        communicationState_{
            RemoteCommunicationState::
                WaitingForData};
};

#endif
