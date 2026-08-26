#include "RemoteVehicleStatus.hpp"

#include "CanProtocol.hpp"

void RemoteVehicleStatus::reset()
{
    hasReceivedValidFrame_ = false;

    systemHealthy_ = false;
    temperatureValid_ = false;
    sensorAAvailable_ = false;
    sensorBAvailable_ = false;

    temperatureDeciCelsius_ = 0;

    faultMask_ = 0U;

    lastReceiveTimeMs_ = 0U;

    validFrameCount_ = 0U;
    invalidFrameCount_ = 0U;

    communicationState_ =
        RemoteCommunicationState::
            WaitingForData;
}

bool RemoteVehicleStatus::processFrame(
    const CanFrame& frame,
    std::uint32_t currentTimeMs)
{
    if (frame.id !=
        CanProtocol::MessageId::
            VehicleHealthStatus)
    {
        ++invalidFrameCount_;
        return false;
    }

    if (frame.length !=
        CanProtocol::VehicleHealthStatus::
            PayloadLength)
    {
        ++invalidFrameCount_;
        return false;
    }

    if (frame.data[
            CanProtocol::
                VehicleHealthStatus::
                    ProtocolVersionIndex] !=
        CanProtocol::ProtocolVersion)
    {
        ++invalidFrameCount_;
        return false;
    }

    const std::uint8_t statusFlags =
        frame.data[
            CanProtocol::
                VehicleHealthStatus::
                    StatusFlagsIndex];

    systemHealthy_ =
        (statusFlags &
         CanProtocol::
             VehicleHealthStatus::
                 SystemHealthyFlag) != 0U;

    temperatureValid_ =
        (statusFlags &
         CanProtocol::
             VehicleHealthStatus::
                 SelectedTemperatureValidFlag) !=
        0U;

    sensorAAvailable_ =
        (statusFlags &
         CanProtocol::
             VehicleHealthStatus::
                 SensorAAvailableFlag) != 0U;

    sensorBAvailable_ =
        (statusFlags &
         CanProtocol::
             VehicleHealthStatus::
                 SensorBAvailableFlag) != 0U;

    const std::uint16_t
        temperatureUnsigned =
            static_cast<std::uint16_t>(
                frame.data[
                    CanProtocol::
                        VehicleHealthStatus::
                            TemperatureLowByteIndex]) |
            static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(
                    frame.data[
                        CanProtocol::
                            VehicleHealthStatus::
                                TemperatureHighByteIndex])
                << 8U);

    temperatureDeciCelsius_ =
        static_cast<std::int16_t>(
            temperatureUnsigned);

    faultMask_ =
        static_cast<std::uint32_t>(
            frame.data[
                CanProtocol::
                    VehicleHealthStatus::
                        FaultMaskByte0Index]) |
        (static_cast<std::uint32_t>(
             frame.data[
                 CanProtocol::
                     VehicleHealthStatus::
                         FaultMaskByte1Index])
         << 8U) |
        (static_cast<std::uint32_t>(
             frame.data[
                 CanProtocol::
                     VehicleHealthStatus::
                         FaultMaskByte2Index])
         << 16U) |
        (static_cast<std::uint32_t>(
             frame.data[
                 CanProtocol::
                     VehicleHealthStatus::
                         FaultMaskByte3Index])
         << 24U);

    hasReceivedValidFrame_ = true;

    lastReceiveTimeMs_ =
        currentTimeMs;

    ++validFrameCount_;

    communicationState_ =
        RemoteCommunicationState::
            Connected;

    return true;
}

void RemoteVehicleStatus::
    updateCommunicationState(
        std::uint32_t currentTimeMs)
{
    if (!hasReceivedValidFrame_)
    {
        communicationState_ =
            RemoteCommunicationState::
                WaitingForData;

        return;
    }

    const std::uint32_t elapsedTimeMs =
        currentTimeMs -
        lastReceiveTimeMs_;

    if (elapsedTimeMs >
        CommunicationTimeoutMs)
    {
        communicationState_ =
            RemoteCommunicationState::
                CommunicationLost;
    }
    else
    {
        communicationState_ =
            RemoteCommunicationState::
                Connected;
    }
}

RemoteCommunicationState
RemoteVehicleStatus::communicationState() const
{
    return communicationState_;
}

bool RemoteVehicleStatus::
    hasReceivedValidFrame() const
{
    return hasReceivedValidFrame_;
}

bool RemoteVehicleStatus::
    systemHealthy() const
{
    return systemHealthy_;
}

bool RemoteVehicleStatus::
    temperatureValid() const
{
    return temperatureValid_;
}

bool RemoteVehicleStatus::
    sensorAAvailable() const
{
    return sensorAAvailable_;
}

bool RemoteVehicleStatus::
    sensorBAvailable() const
{
    return sensorBAvailable_;
}

std::int16_t
RemoteVehicleStatus::
    temperatureDeciCelsius() const
{
    return temperatureDeciCelsius_;
}

std::uint32_t
RemoteVehicleStatus::faultMask() const
{
    return faultMask_;
}

std::uint32_t
RemoteVehicleStatus::
    lastReceiveTimeMs() const
{
    return lastReceiveTimeMs_;
}

std::uint32_t
RemoteVehicleStatus::
    validFrameCount() const
{
    return validFrameCount_;
}

std::uint32_t
RemoteVehicleStatus::
    invalidFrameCount() const
{
    return invalidFrameCount_;
}
