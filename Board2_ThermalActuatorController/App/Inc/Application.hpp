#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "ActuatorCommandPolicy.hpp"
#include "CanBus.hpp"
#include "RemoteVehicleStatus.hpp"
#include "ThermalControlStateMachine.hpp"

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

    void updateThermalControlState();

    void reportRemoteVehicleStatus();

    void reportCommunicationState(
        RemoteCommunicationState state);

    void reportThermalControlState(
        ThermalControlState state);

    void reportActuatorCommand();

    void runRemoteStatusSelfTest();

    void runThermalControlSelfTest();

    void runActuatorCommandSelfTest();

    void transmitText(
        const char* text);

    CanBus canBus_;

    RemoteVehicleStatus
        remoteVehicleStatus_;

    ThermalControlStateMachine
        thermalControlStateMachine_;

    ActuatorCommandPolicy
        actuatorCommandPolicy_;

    RemoteCommunicationState
        previousCommunicationState_{
            RemoteCommunicationState::
                WaitingForData};

    ThermalControlState
        previousThermalControlState_{
            ThermalControlState::Safe};

    bool communicationStateInitialized_{
        false};

    bool thermalControlStateInitialized_{
        false};
};

#endif
