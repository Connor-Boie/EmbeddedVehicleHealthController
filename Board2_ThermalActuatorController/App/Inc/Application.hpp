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

    void updateUserButton(
        std::uint32_t currentTimeMs);

    void startActuatorSelfTest(
        std::uint32_t currentTimeMs);

    void updateActuatorSelfTest(
        std::uint32_t currentTimeMs);

    void applyActiveOutputCommand();

    void updateBuzzerPatternTiming();

    [[nodiscard]] bool
        readUserButtonPressed() const;

    [[nodiscard]] ActuatorCommand
        actuatorSelfTestCommand(
            std::uint8_t stage) const;

    void reportActuatorSelfTestStage();

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

    ActuatorCommand activeOutputCommand_{};

    RemoteCommunicationState
        previousCommunicationState_{
            RemoteCommunicationState::
                WaitingForData};

    ThermalControlState
        previousThermalControlState_{
            ThermalControlState::Safe};

    bool communicationStateInitialized_{false};
    bool thermalControlStateInitialized_{false};

    bool startupGraceActive_{true};
    std::uint32_t startupTimeMs_{0U};

    bool userButtonRawPressed_{false};
    bool userButtonDebouncedPressed_{false};
    bool userButtonArmed_{false};
    std::uint32_t
        userButtonRawChangeTimeMs_{0U};
    std::uint32_t
        userButtonReleaseStartTimeMs_{0U};

    bool actuatorSelfTestActive_{false};
    std::uint8_t actuatorSelfTestStage_{0U};
    std::uint32_t
        actuatorSelfTestStageStartTimeMs_{0U};
};

#endif
