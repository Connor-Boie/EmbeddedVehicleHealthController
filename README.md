# Embedded Vehicle Health Controller

A bare-metal embedded C++ firmware project for the
STMicroelectronics NUCLEO-F446RE development board.

The project simulates a small vehicle subsystem controller that collects
sensor data, tracks system state, detects and reports faults, receives
commands, and transmits telemetry.

## Project Status

Initial board bring-up, C++ application integration, and GPIO output
abstraction are complete.

Current capabilities include:

- STM32F446RE hardware initialization generated through STM32CubeMX
- Firmware builds and flashing through STM32CubeIDE
- Onboard ST-LINK programming and debugging
- A user-owned C++ application layer
- A C-compatible bridge between generated C code and C++ code
- A reusable C++ digital-output abstraction
- Onboard LD2 status LED control
- Debug inspection of C++ object state

The current LED blink verifies the hardware, toolchain, mixed C/C++ build,
and GPIO abstraction. Blocking timing will later be replaced with
non-blocking application timing.

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

After the firmware starts:

1. The STM32 HAL is initialized.
2. The system clock is configured.
3. Generated GPIO and board-support initialization runs.
4. Generated C code initializes the C++ application through a C-compatible
   bridge.
5. The C++ application initializes its status LED abstraction.
6. The main loop repeatedly calls the C++ application.
7. The application toggles the onboard status LED.
8. A C++ member variable records the number of LED toggles.
9. The firmware waits approximately 500 milliseconds between toggles.

The current implementation intentionally uses `HAL_Delay()` while the
application architecture is established.

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

A C-compatible bridge connects the generated C entry point to the C++
application:

```text
App/Inc/application_bridge.h
App/Src/application_bridge.cpp
```

GPIO output operations are isolated in:

```text
App/Inc/DigitalOutput.hpp
App/Src/DigitalOutput.cpp
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
    v
DigitalOutput::turnOff()

main.c infinite loop
    |
    v
application_run()
    |
    v
Application::run()
    |
    v
DigitalOutput::toggle()
    |
    v
STM32 HAL GPIO function
```

## Repository Structure

```text
EmbeddedVehicleHealthController/
├── App/
│   ├── Inc/
│   │   ├── Application.hpp
│   │   ├── DigitalOutput.hpp
│   │   └── application_bridge.h
│   └── Src/
│       ├── Application.cpp
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

It provides these operations:

```cpp
void turnOn();
void turnOff();
void toggle();
void set(bool enabled);
```

The `Application` class owns a `DigitalOutput` representing the onboard
status LED:

```cpp
DigitalOutput statusLed_;
```

This is composition: the application has a digital output that it uses as
its status LED.

The application no longer directly calls STM32 GPIO functions. It requests
operations from the digital-output abstraction instead.

## C and C++ Integration

STM32CubeMX generates the firmware entry point and hardware initialization in
C. The main application and hardware abstractions are implemented in C++.

The bridge functions use C-compatible linkage:

```cpp
extern "C" void application_init(void);
extern "C" void application_run(void);
```

This prevents C++ name mangling from preventing the generated C entry point
from linking to functions implemented in C++.

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
11. Verify that LD2 toggles approximately every 500 milliseconds.

## Debugging

To inspect the application:

1. Set a breakpoint on `statusLed_.toggle()` in `Application.cpp`.
2. Start a debug session.
3. Resume execution.
4. Expand `this` in the Variables view.
5. Inspect `statusLed_` and `blinkCount_`.
6. Step into `DigitalOutput::toggle()`.
7. Inspect the output object's `port_` and `pin_` members.
8. Resume repeatedly and verify that `blinkCount_` increases.

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
- Prefer composition over inheritance for application components.
- Prefer fixed-size and statically allocated resources.
- Avoid runtime dynamic allocation where practical.
- Keep interrupt handlers short.
- Prefer non-blocking timing for normal application behavior.
- Separate hardware-dependent code from testable system logic.
- Add abstractions only when they improve clarity or testability.
- Validate communication input before processing commands.

## Planned Features

The firmware will be expanded to include:

- Button input and software debouncing
- Non-blocking timing
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
- RTOS: Not currently used
- Dynamic allocation: Avoided during normal firmware operation