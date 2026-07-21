# Embedded Vehicle Health Controller

A bare-metal embedded C++ firmware project for the
STMicroelectronics NUCLEO-F446RE development board.

The project simulates a small vehicle subsystem controller that collects
sensor data, tracks system state, detects and reports faults, receives
commands, and transmits telemetry.

## Project Status

Initial board bring-up, C++ application integration, GPIO output control,
and debounced user-button input are complete.

Current capabilities include:

- STM32F446RE hardware initialization generated through STM32CubeMX
- Firmware builds and flashing through STM32CubeIDE
- Onboard ST-LINK programming and debugging
- A user-owned C++ application layer
- A C-compatible bridge between generated C code and C++ code
- A reusable C++ digital-output abstraction
- Polled onboard user-button input
- Hardware-independent software debouncing
- One-time pressed and released button events
- Onboard status LED control from confirmed button presses
- Debug inspection of nested C++ object state

Each confirmed press of the onboard USER button toggles LD2 once. Holding
the button does not repeatedly toggle the LED.

## Target Hardware

- Development board: NUCLEO-F446RE
- Microcontroller: STM32F446RE
- Processor core: Arm Cortex-M4
- Onboard debugger/programmer: ST-LINK
- Onboard user LED: LD2 on PA5
- Onboard user button: B1 on PC13

## Development Tools

- STM32CubeMX
- STM32CubeIDE
- STM32CubeF4 firmware package
- Arm GNU toolchain
- Git

STM32CubeMX is used for hardware configuration and generated initialization
code. STM32CubeIDE is used for editing, building, flashing, and debugging.

The project uses a mixed C/C++ build:

```text
.c files   → compiled as C
.cpp files → compiled as C++
```

## Current Behavior

After firmware startup:

1. The STM32 HAL is initialized.
2. The system clock is configured.
3. Generated GPIO and board-support initialization runs.
4. Generated C code initializes the C++ application through a C-compatible
   bridge.
5. The application initializes the status LED and button debouncer.
6. The main loop repeatedly samples the user button.
7. Raw button readings are passed to the software debouncer.
8. A state change is accepted only after remaining stable for 30
   milliseconds.
9. A confirmed button press generates one pressed event.
10. Each pressed event toggles the onboard LD2 status LED.
11. A C++ member variable records the number of confirmed button presses.

## Software Architecture

The generated STM32 entry point remains in C:

```text
Core/Src/main.c
```

User-owned application behavior is implemented in C++:

```text
App/Inc/Application.hpp
App/Src/Application.cpp
```

A C-compatible bridge connects generated C code to the C++ application:

```text
App/Inc/application_bridge.h
App/Src/application_bridge.cpp
```

GPIO output operations are isolated in:

```text
App/Inc/DigitalOutput.hpp
App/Src/DigitalOutput.cpp
```

Hardware-independent button debouncing is implemented in:

```text
App/Inc/ButtonDebouncer.hpp
App/Src/ButtonDebouncer.cpp
```

The current execution flow is:

```text
main.c
    |
    v
application_init()
    |
    v
Application::initialize()
    |
    +--> DigitalOutput::turnOff()
    |
    +--> Read initial button state
    |
    +--> ButtonDebouncer::initialize()

main.c infinite loop
    |
    v
application_run()
    |
    v
Application::run()
    |
    +--> Read raw button state
    |
    +--> ButtonDebouncer::update()
    |
    +--> Check pressedEvent()
    |
    +--> DigitalOutput::toggle()
```

## Repository Structure

```text
EmbeddedVehicleHealthController/
├── App/
│   ├── Inc/
│   │   ├── Application.hpp
│   │   ├── ButtonDebouncer.hpp
│   │   ├── DigitalOutput.hpp
│   │   └── application_bridge.h
│   └── Src/
│       ├── Application.cpp
│       ├── ButtonDebouncer.cpp
│       ├── DigitalOutput.cpp
│       └── application_bridge.cpp
├── Core/
│   ├── Inc/
│   └── Src/
├── Drivers/
│   ├── CMSIS/
│   └── STM32F4xx_HAL_Driver/
├── EmbeddedVehicleHealthController.ioc
├── STM32F446RETX_FLASH.ld
├── startup_stm32f446retx.s
├── .gitignore
└── README.md
```

Generated build and IDE metadata files may vary with installed STM32 tool
versions.

## GPIO Output Abstraction

`DigitalOutput` represents a GPIO configured as a digital output.

It stores:

- A pointer to the STM32 GPIO port
- The GPIO pin mask

It provides:

```cpp
void turnOn();
void turnOff();
void toggle();
void set(bool enabled);
```

The `Application` class owns a `DigitalOutput` representing the onboard
status LED.

## Button Debouncing

Physical button contacts can rapidly alternate between pressed and released
when they first make or break contact.

`ButtonDebouncer` separates:

- Raw sampled state
- Confirmed stable state
- Pressed transition events
- Released transition events

The raw state must remain stable for the configured debounce period before
the confirmed state changes.

The current debounce period is:

```text
30 milliseconds
```

A pressed event is generated once when the confirmed state changes from
released to pressed. Holding the button does not generate repeated pressed
events.

## State and Event Semantics

The debouncer provides both state and events:

```cpp
bool isPressed() const;
bool pressedEvent() const;
bool releasedEvent() const;
```

`isPressed()` describes the current confirmed state.

`pressedEvent()` and `releasedEvent()` describe transitions that occurred
during the latest update.

This allows the application to toggle the status LED once for each confirmed
button press.

## C and C++ Integration

STM32CubeMX generates the firmware entry point and hardware initialization in
C. The main application and reusable software components are implemented in
C++.

The bridge functions use C-compatible linkage:

```cpp
extern "C" void application_init(void);
extern "C" void application_run(void);
```

This prevents C++ name mangling from preventing generated C code from linking
to functions implemented in C++.

## Build and Run

1. Open `EmbeddedVehicleHealthController.ioc` in STM32CubeMX when hardware
   configuration changes are required.
2. Generate code for the STM32CubeIDE toolchain.
3. Open the project in STM32CubeIDE.
4. Verify that the project is configured as a mixed C/C++ project.
5. Verify that `App/Inc` is included in the C and C++ compiler include paths.
6. Verify that `App/Src` is included in the build.
7. Build the project.
8. Connect the NUCLEO-F446RE through the ST-LINK USB connector.
9. Start a debug session or run the firmware.
10. Resume execution if the debugger pauses at `main()`.
11. Verify that LD2 starts off.
12. Press the onboard USER button and confirm that LD2 toggles once.
13. Hold the button and confirm that LD2 does not repeatedly toggle.
14. Press the button repeatedly and confirm one toggle per press.

## Debugging

To inspect button processing:

1. Set a breakpoint on `statusLed_.toggle()` in `Application.cpp`.
2. Start a debug session.
3. Resume execution.
4. Press the onboard USER button.
5. Inspect `this->buttonPressCount_`.
6. Expand `this->buttonDebouncer_`.
7. Inspect the raw and confirmed button states.
8. Resume and verify that the counter increases once per press.

Useful members include:

```text
rawPressed_
confirmedPressed_
pressedEvent_
releasedEvent_
lastRawChangeTimeMs_
```

## Generated-Code Policy

STM32CubeMX-generated files may be regenerated.

Manual changes to generated files must be placed only inside protected
sections marked with:

```c
/* USER CODE BEGIN ... */

/* USER CODE END ... */
```

Application behavior is placed in user-owned files under `App/` rather than
directly in generated files.

Generated initialization calls outside protected regions are not manually
moved or edited.

## Design Principles

- Keep generated hardware initialization separate from application logic.
- Keep the generated firmware entry point small.
- Implement application behavior in user-owned C++ classes.
- Encapsulate repeated hardware operations behind focused abstractions.
- Separate raw hardware input from confirmed application state.
- Represent one-time transitions as events.
- Keep hardware-independent algorithms testable without the board.
- Prefer composition over inheritance for application components.
- Prefer fixed-size and statically allocated resources.
- Avoid runtime dynamic allocation where practical.
- Keep interrupt handlers short.
- Prefer non-blocking timing for normal application behavior.
- Add abstractions only when they improve clarity or testability.
- Validate communication input before processing commands.

## Planned Features

The firmware will be expanded to include:

- Fully non-blocking application timing
- Periodic task execution
- Hardware timer interrupts
- Cooperative task scheduling
- UART telemetry and command handling
- Sensor interfaces
- ADC and I2C sensor input
- System operating modes
- Fault detection and fault management
- Watchdog recovery
- CAN communication
- Host-based unit tests
- Static analysis and additional design documentation

## Project Type

- Category: Embedded firmware
- Architecture: Bare-metal
- Primary application language: Modern C++
- Generated hardware-support language: C
- Scheduling model: Cooperative
- Input model: Polling with software debouncing
- RTOS: Not currently used
- Dynamic allocation: Avoided during normal firmware operation