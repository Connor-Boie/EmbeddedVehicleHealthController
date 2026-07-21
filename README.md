# Embedded Vehicle Health Controller

A bare-metal embedded C++ firmware project for the
STMicroelectronics NUCLEO-F446RE development board.

The project simulates a small vehicle subsystem controller that collects
sensor data, tracks system state, detects and reports faults, receives
commands, and transmits telemetry.

## Project Status

The initial board bring-up and C++ application integration are complete.

Current capabilities include:

- STM32F446RE hardware initialization generated through STM32CubeMX
- Firmware builds and flashing through STM32CubeIDE
- Onboard ST-LINK programming and debugging
- Onboard LD2 status LED control
- A user-owned C++ application class
- A C-compatible bridge between generated C code and C++ application code
- Debug inspection of C++ object state

The current LED blink is a hardware, toolchain, and language-integration
verification test. Blocking timing will later be replaced with non-blocking
application timing.

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

## Current Behavior

After the firmware starts:

1. The STM32 HAL is initialized.
2. The system clock is configured.
3. The onboard LED GPIO is initialized.
4. Generated C code initializes the C++ application through a C-compatible
   bridge.
5. The main loop repeatedly calls the C++ application.
6. The application toggles the onboard LED.
7. A C++ member variable records the number of LED toggles.
8. The firmware waits approximately 500 milliseconds between toggles.

The current implementation intentionally uses `HAL_Delay()` while the
application structure is established.

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

A C-compatible bridge connects the two layers:

```text
App/Inc/application_bridge.h
App/Src/application_bridge.cpp
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

main.c infinite loop
    |
    v
application_run()
    |
    v
Application::run()
```

This structure keeps STM32CubeMX-generated initialization separate from
application behavior.

## Repository Structure

```text
EmbeddedVehicleHealthController/
├── App/
│   ├── Inc/
│   │   ├── Application.hpp
│   │   └── application_bridge.h
│   └── Src/
│       ├── Application.cpp
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

## C and C++ Integration

STM32CubeMX generates the firmware entry point and hardware initialization in
C. The main application is implemented in C++.

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
4. Verify that `App/Inc` is included in both the C and C++ compiler include
   paths.
5. Verify that `App/Src` is included in the build.
6. Build the project.
7. Connect the NUCLEO-F446RE through the ST-LINK USB connector.
8. Start a debug session or run the firmware.
9. Resume execution if the debugger pauses at `main()`.
10. Verify that LD2 toggles approximately every 500 milliseconds.

## Debugging

To inspect the C++ application state:

1. Set a breakpoint on `++blinkCount_` in `Application.cpp`.
2. Start a debug session.
3. Resume execution.
4. Expand `this` in the Variables view.
5. Inspect `blinkCount_`.
6. Alternatively, add `this->blinkCount_` to the Expressions view.
7. Resume repeatedly and verify that the value increases.

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

## Design Principles

- Keep generated hardware initialization separate from application logic.
- Keep the generated firmware entry point small.
- Implement application behavior in user-owned C++ classes.
- Prefer fixed-size and statically allocated resources.
- Avoid runtime dynamic allocation where practical.
- Keep interrupt handlers short.
- Prefer non-blocking timing for normal application behavior.
- Separate hardware-dependent code from testable system logic.
- Add abstractions only when they improve clarity or testability.
- Validate communication input before processing commands.

## Planned Features

The firmware will be expanded to include:

- GPIO output abstraction
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