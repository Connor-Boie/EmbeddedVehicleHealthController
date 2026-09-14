#ifndef REMOTE_ACTUATOR_STATUS_HPP
#define REMOTE_ACTUATOR_STATUS_HPP

#include "CanBus.hpp"

#include <cstdint>

enum class RemoteActuatorCommunicationState
{
    WaitingForData,
    Connected,
    CommunicationLost
};

class RemoteActuatorStatus
{
public:
    static constexpr std::uint32_t
        CommunicationTimeoutMs = 1500U;

    void reset();

    [[nodiscard]] bool processFrame(
        const CanFrame& frame,
        std::uint32_t currentTimeMs);

    void updateCommunicationState(
        std::uint32_t currentTimeMs);

    [[nodiscard]] bool
        hasReceivedValidFrame() const;

    [[nodiscard]] std::uint32_t
        validFrameCount() const;

    [[nodiscard]] bool
        controllerOperational() const;

    [[nodiscard]] bool
        vehicleDataConnected() const;

    [[nodiscard]] bool
        selfTestActive() const;

    [[nodiscard]] bool
        safeState() const;

    [[nodiscard]] std::uint8_t
        thermalStateCode() const;

    [[nodiscard]] std::uint8_t
        coolingDutyPercent() const;

    [[nodiscard]] std::uint8_t
        warningColorCode() const;

    [[nodiscard]] std::uint8_t
        buzzerPatternCode() const;

    [[nodiscard]] RemoteActuatorCommunicationState
        communicationState() const;

private:
    bool hasReceivedValidFrame_{false};
    std::uint32_t validFrameCount_{0U};
    std::uint32_t lastValidFrameTimeMs_{0U};

    bool controllerOperational_{false};
    bool vehicleDataConnected_{false};
    bool selfTestActive_{false};
    bool safeState_{true};

    std::uint8_t thermalStateCode_{0U};
    std::uint8_t coolingDutyPercent_{0U};
    std::uint8_t warningColorCode_{0U};
    std::uint8_t buzzerPatternCode_{0U};

    RemoteActuatorCommunicationState
        communicationState_{
            RemoteActuatorCommunicationState::
                WaitingForData};
};

#endif
