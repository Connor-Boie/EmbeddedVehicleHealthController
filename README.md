# Embedded Vehicle Health Controller

A bare-metal embedded C++ firmware project for the
STMicroelectronics NUCLEO-F446RE development board.

The project simulates a small vehicle subsystem controller that collects
sensor data, tracks system state, detects and reports faults, receives
commands, and transmits telemetry.

## Project Status

Initial board bring-up is complete:

- Created the NUCLEO-F446RE firmware project.
- Generated the initial STM32 hardware configuration.
- Configured the onboard green user LED.
- Built and flashed the firmware through the onboard ST-LINK debugger.
- Verified the target using a blocking LED-blink test.
- Used a breakpoint and watch expression to inspect firmware state.

The current LED blink is a hardware and toolchain verification test.
It will later be replaced with non-blocking application timing.

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
4. The main loop toggles the onboard LED.
5. The firmware waits approximately 500 milliseconds between toggles.
6. A debug counter records how many toggles have occurred.

The current implementation intentionally uses `HAL_Delay()` because the
firmware is currently limited to verifying the board and development
toolchain.

## Planned Features

The firmware will be expanded to include:

- C++ application architecture
- GPIO abstractions
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
- Static analysis and documentation

## Initial Architecture

The current project mainly consists of STM32-generated startup,
initialization, and HAL support code.

The firmware will be separated into:

- Generated STM32 initialization code
- User-owned C++ application code
- Hardware interface classes
- Testable system logic
- Communication and protocol components

## Repository Structure

```text
EmbeddedVehicleHealthController/
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

The exact generated build and IDE metadata files may vary with the installed
STM32CubeMX and STM32CubeIDE versions.

## Build and Run

1. Open `EmbeddedVehicleHealthController.ioc` in STM32CubeMX when hardware
   configuration changes are required.
2. Generate code for the STM32CubeIDE toolchain.
3. Open the generated project in STM32CubeIDE.
4. Build the project.
5. Connect the NUCLEO-F446RE through the ST-LINK USB connector.
6. Start a debug session or run the firmware.
7. Resume execution if the debugger pauses at `main()`.
8. Verify that the green user LED toggles approximately every 500
   milliseconds.

## Debugging

The current firmware includes a file-scope counter named `blinkCount`.

To inspect it:

1. Set a breakpoint on `++blinkCount`.
2. Start a debug session.
3. Add `blinkCount` to the Expressions view.
4. Resume execution repeatedly.
5. Verify that the counter increases each time the breakpoint is reached.

## Generated-Code Policy

STM32CubeMX-generated files may be regenerated.

Manual changes to generated files must be placed only inside protected
sections marked with:

```c
/* USER CODE BEGIN ... */

/* USER CODE END ... */
```

Most application logic will be placed in user-owned C++ files rather than
directly in generated files.

## Design Principles

The project follows these principles:

- Keep generated hardware initialization separate from application logic.
- Prefer fixed-size and statically allocated resources.
- Avoid runtime dynamic allocation where practical.
- Keep interrupt handlers short.
- Prefer non-blocking timing for normal application behavior.
- Separate hardware-dependent code from testable system logic.
- Add abstractions only when they improve clarity or testability.
- Validate communication input before processing commands.

## Project Type

- Category: Embedded firmware
- Architecture: Bare-metal
- Primary application language: Modern C++
- Generated hardware-support language: C
- Scheduling model: Cooperative
- RTOS: Not currently used
- Dynamic allocation: Avoided during normal firmware operation