#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "CanBus.hpp"
#include "RemoteVehicleStatus.hpp"

#include <cstdint>

class Application
{
public:
    Application();

    void initialize();

    void run();

private:
    void processCanReceive();

    void updateRemoteCommunicationState();

    void reportRemoteVehicleStatus();

    void reportCommunicationState(
        RemoteCommunicationState state);

    void runRemoteStatusSelfTest();

    void transmitText(
        const char* text);

    CanBus canBus_;

    RemoteVehicleStatus
        remoteVehicleStatus_;

    RemoteCommunicationState
        previousCommunicationState_{
            RemoteCommunicationState::
                WaitingForData};

    bool communicationStateInitialized_{
        false};
};

#endif
