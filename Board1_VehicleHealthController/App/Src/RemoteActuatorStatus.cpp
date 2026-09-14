#include "RemoteActuatorStatus.hpp"

#include "CanProtocol.hpp"

void RemoteActuatorStatus::reset()
{
    hasReceivedValidFrame_ = false;
    validFrameCount_ = 0U;
    lastValidFrameTimeMs_ = 0U;

    controllerOperational_ = false;
    vehicleDataConnected_ = false;
    selfTestActive_ = false;
    safeState_ = true;

    thermalStateCode_ =
        CanProtocol::ThermalActuatorStatus::
            ThermalState::Safe;

    coolingDutyPercent_ = 100U;

    warningColorCode_ =
        CanProtocol::ThermalActuatorStatus::
            WarningColor::Magenta;

    buzzerPatternCode_ =
        CanProtocol::ThermalActuatorStatus::
            BuzzerPattern::Fault;

    communicationState_ =
        RemoteActuatorCommunicationState::
            WaitingForData;
}

bool RemoteActuatorStatus::processFrame(
    const CanFrame& frame,
    std::uint32_t currentTimeMs)
{
    if (frame.id !=
        CanProtocol::MessageId::
            ThermalActuatorStatus)
    {
        return false;
    }

    if (frame.length !=
        CanProtocol::ThermalActuatorStatus::
            PayloadLength)
    {
        return false;
    }

    if (frame.data[
            CanProtocol::ThermalActuatorStatus::
                ProtocolVersionIndex] !=
        CanProtocol::ProtocolVersion)
    {
        return false;
    }

    const std::uint8_t thermalStateCode =
        frame.data[
            CanProtocol::ThermalActuatorStatus::
                ThermalStateIndex];

    const std::uint8_t coolingDutyPercent =
        frame.data[
            CanProtocol::ThermalActuatorStatus::
                CoolingDutyPercentIndex];

    const std::uint8_t warningColorCode =
        frame.data[
            CanProtocol::ThermalActuatorStatus::
                WarningColorIndex];

    const std::uint8_t buzzerPatternCode =
        frame.data[
            CanProtocol::ThermalActuatorStatus::
                BuzzerPatternIndex];

    if (thermalStateCode >
        CanProtocol::ThermalActuatorStatus::
            ThermalState::Safe)
    {
        return false;
    }

    if (coolingDutyPercent > 100U)
    {
        return false;
    }

    if (warningColorCode >
        CanProtocol::ThermalActuatorStatus::
            WarningColor::Magenta)
    {
        return false;
    }

    if (buzzerPatternCode >
        CanProtocol::ThermalActuatorStatus::
            BuzzerPattern::Fault)
    {
        return false;
    }

    const std::uint8_t statusFlags =
        frame.data[
            CanProtocol::ThermalActuatorStatus::
                StatusFlagsIndex];

    controllerOperational_ =
        (statusFlags &
         CanProtocol::ThermalActuatorStatus::
             ControllerOperationalFlag) != 0U;

    vehicleDataConnected_ =
        (statusFlags &
         CanProtocol::ThermalActuatorStatus::
             VehicleDataConnectedFlag) != 0U;

    selfTestActive_ =
        (statusFlags &
         CanProtocol::ThermalActuatorStatus::
             SelfTestActiveFlag) != 0U;

    safeState_ =
        (statusFlags &
         CanProtocol::ThermalActuatorStatus::
             SafeStateFlag) != 0U;

    thermalStateCode_ =
        thermalStateCode;

    coolingDutyPercent_ =
        coolingDutyPercent;

    warningColorCode_ =
        warningColorCode;

    buzzerPatternCode_ =
        buzzerPatternCode;

    hasReceivedValidFrame_ = true;

    ++validFrameCount_;

    lastValidFrameTimeMs_ =
        currentTimeMs;

    communicationState_ =
        RemoteActuatorCommunicationState::
            Connected;

    return true;
}

void RemoteActuatorStatus::
    updateCommunicationState(
        std::uint32_t currentTimeMs)
{
    if (!hasReceivedValidFrame_)
    {
        communicationState_ =
            RemoteActuatorCommunicationState::
                WaitingForData;

        return;
    }

    const std::uint32_t elapsedTimeMs =
        currentTimeMs -
        lastValidFrameTimeMs_;

    if (elapsedTimeMs >
        CommunicationTimeoutMs)
    {
        communicationState_ =
            RemoteActuatorCommunicationState::
                CommunicationLost;
    }
    else
    {
        communicationState_ =
            RemoteActuatorCommunicationState::
                Connected;
    }
}

bool RemoteActuatorStatus::
    hasReceivedValidFrame() const
{
    return hasReceivedValidFrame_;
}

std::uint32_t
RemoteActuatorStatus::validFrameCount() const
{
    return validFrameCount_;
}

bool RemoteActuatorStatus::controllerOperational() const
{
    return controllerOperational_;
}

bool RemoteActuatorStatus::
    vehicleDataConnected() const
{
    return vehicleDataConnected_;
}

bool RemoteActuatorStatus::selfTestActive() const
{
    return selfTestActive_;
}

bool RemoteActuatorStatus::safeState() const
{
    return safeState_;
}

std::uint8_t
RemoteActuatorStatus::thermalStateCode() const
{
    return thermalStateCode_;
}

std::uint8_t
RemoteActuatorStatus::coolingDutyPercent() const
{
    return coolingDutyPercent_;
}

std::uint8_t
RemoteActuatorStatus::warningColorCode() const
{
    return warningColorCode_;
}

std::uint8_t
RemoteActuatorStatus::buzzerPatternCode() const
{
    return buzzerPatternCode_;
}

RemoteActuatorCommunicationState
RemoteActuatorStatus::communicationState() const
{
    return communicationState_;
}
