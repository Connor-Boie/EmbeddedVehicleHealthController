#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "ActuatorCommandPolicy.hpp"
#include "BuzzerPatternSequencer.hpp"
#include "BuzzerPwm.hpp"
#include "CanBus.hpp"
#include "FanPwm.hpp"
#include "RemoteVehicleStatus.hpp"
#include "RgbLedPwm.hpp"
#include "ThermalControlStateMachine.hpp"

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

    void updateBuzzerPatternTiming();

    void reportRemoteVehicleStatus();

    void reportCommunicationState(
        RemoteCommunicationState state);

    void reportThermalControlState(
        ThermalControlState state);

    void reportActuatorCommand();

    void reportBuzzerTimingState();

    void reportFanPwmState();

    void runRemoteStatusSelfTest();

    void runThermalControlSelfTest();

    void runActuatorCommandSelfTest();

    void runBuzzerTimingSelfTest();

    void runRgbMappingSelfTest();

    void runRgbHardwareSelfTest();

    void runFanHardwareSelfTest();

    void transmitText(
        const char* text);

    CanBus canBus_;
    RgbLedPwm rgbLed_;
    FanPwm fanPwm_;
    BuzzerPwm buzzerPwm_;

    RemoteVehicleStatus remoteVehicleStatus_{};
    ThermalControlStateMachine
        thermalControlStateMachine_{};
    ActuatorCommandPolicy
        actuatorCommandPolicy_{};
    BuzzerPatternSequencer
        buzzerPatternSequencer_{};

    RemoteCommunicationState
        previousCommunicationState_{
            RemoteCommunicationState::
                WaitingForData};

    ThermalControlState
        previousThermalControlState_{
            ThermalControlState::Safe};

    bool communicationStateInitialized_{false};
    bool thermalControlStateInitialized_{false};
};

#endif
