# Embedded Vehicle Health Controller

A bare-metal embedded C++ firmware project for the
STMicroelectronics NUCLEO-F446RE development board.

The project simulates a small vehicle subsystem controller that collects
sensor data, tracks system state, detects and reports faults, receives
commands, and transmits telemetry.

## Project Status

Initial board bring-up, mixed C/C++ application integration, GPIO output
control, debounced user-button input, non-blocking timing, and cooperative
periodic task execution are complete.

Current capabilities include:

- STM32F446RE hardware initialization generated through STM32CubeMX
- Firmware builds, flashing, and debugging through STM32CubeIDE
- Onboard ST-LINK programming and debugging
- A user-owned C++ application layer
- A C-compatible bridge between generated C and C++ application code
- A reusable digital-output abstraction
- Hardware-independent button-debouncing logic
- One-time pressed and released button events
- A reusable rollover-safe periodic timer
- Multiple independent cooperative periodic tasks
- A configurable status heartbeat
- Lightweight task-liveness monitoring
- Debug inspection of nested C++ application state

The application currently runs three periodic activities:

```text
Button sampling task    every 5 ms
Heartbeat task          every 500 ms
Health-monitoring task  every 1000 ms
```

The USER button enables or disables the status heartbeat. The main
application loop does not use blocking delays.

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
5. The application initializes its digital output, button debouncer, and
   periodic timers.
6. The generated main loop repeatedly calls `Application::run()`.
7. The application checks each periodic timer.
8. The button task samples and debounces the USER button every 5
   milliseconds.
9. The heartbeat task toggles LD2 every 500 milliseconds while enabled.
10. The health task evaluates button-task liveness every 1000 milliseconds.
11. Each confirmed USER-button press enables or disables the heartbeat.
12. Disabling the heartbeat turns LD2 off.
13. Re-enabling the heartbeat resumes periodic LED toggling.

## Cooperative Tasks

### Button task

Period:

```text
5 milliseconds
```

Responsibilities:

- Read the raw USER-button state
- Update the software debouncer
- Generate confirmed button events
- Toggle heartbeat enable state
- Record button-task execution time
- Count confirmed button presses

### Heartbeat task

Period:

```text
500 milliseconds
```

Responsibilities:

- Check whether the heartbeat is enabled
- Toggle the onboard status LED
- Count heartbeat-task executions

### Health-monitoring task

Period:

```text
1000 milliseconds
```

Responsibilities:

- Calculate the time since the button task last executed
- Compare task age against a 50-millisecond timeout
- Update the current application health state
- Count health evaluations

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

Reusable periodic timing is implemented in:

```text
App/Inc/PeriodicTimer.hpp
App/Src/PeriodicTimer.cpp
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
    +--> Initialize status LED
    +--> Initialize button debouncer
    +--> Initialize button timer
    +--> Initialize heartbeat timer
    +--> Initialize health timer

main.c infinite loop
    |
    v
application_run()
    |
    v
Application::run()
    |
    +--> Button timer due?
    |        |
    |        +--> Application::processButton()
    |
    +--> Heartbeat timer due?
    |        |
    |        +--> Application::updateHeartbeat()
    |
    +--> Health timer due?
             |
             +--> Application::performHealthCheck()
```

## Repository Structure

```text
EmbeddedVehicleHealthController/
├── App/
│   ├── Inc/
│   │   ├── Application.hpp
│   │   ├── ButtonDebouncer.hpp
│   │   ├── DigitalOutput.hpp
│   │   ├── PeriodicTimer.hpp
│   │   └── application_bridge.h
│   └── Src/
│       ├── Application.cpp
│       ├── ButtonDebouncer.cpp
│       ├── DigitalOutput.cpp
│       ├── PeriodicTimer.cpp
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

## Periodic Timing

`PeriodicTimer` determines whether a configured amount of time has elapsed.

It stores:

- The configured period in milliseconds
- The timestamp associated with the previous execution

It provides:

```cpp
void initialize(std::uint32_t currentTimeMs);
bool isDue(std::uint32_t currentTimeMs);
```

The timer uses unsigned subtraction:

```cpp
currentTimeMs - previousExecutionTimeMs_
```

This supports normal 32-bit millisecond-counter rollover.

Each application task owns an independent timer period.

## Task Liveness Monitoring

The button task records its latest execution time:

```cpp
lastButtonTaskTimeMs_ = currentTimeMs;
```

The health task calculates:

```cpp
currentTimeMs - lastButtonTaskTimeMs_
```

The application currently considers the task healthy when its age is no
greater than:

```text
50 milliseconds
```

This establishes a basic software-supervision pattern that can later be
extended to sensor, communication, and telemetry tasks.

## Button Debouncing

The button-sampling period is:

```text
5 milliseconds
```

The debounce stability period is:

```text
30 milliseconds
```

A raw state must remain stable for the full debounce period before it becomes
the confirmed application state.

A pressed event is generated once when the confirmed state changes from
released to pressed.

## Status Heartbeat

The heartbeat period is:

```text
500 milliseconds
```

While heartbeat operation is enabled, the status LED toggles every heartbeat
period.

A confirmed USER-button press reverses the heartbeat enable state:

```text
Enabled  → disabled
Disabled → enabled
```

When disabled, the status LED is explicitly turned off.

## Cooperative Execution

The firmware currently uses one main execution context.

`Application::run()` checks which tasks are due and executes them in this
order:

```text
1. Button task
2. Heartbeat task
3. Health task
```

Each task is expected to:

- Avoid blocking
- Complete quickly
- Preserve required state between calls
- Return control to the main loop

No RTOS is currently used.

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
11. Verify that LD2 begins blinking.
12. Press the USER button and verify that the heartbeat stops.
13. Confirm that LD2 remains off while the heartbeat is disabled.
14. Press the USER button again and verify that the heartbeat resumes.
15. Hold the button and verify that only one state change occurs.

## Debugging

Useful application values include:

```text
buttonPressCount_
heartbeatExecutionCount_
healthCheckCount_
lastButtonTaskTimeMs_
heartbeatEnabled_
systemHealthy_
```

To inspect cooperative task behavior:

1. Set a breakpoint in `Application::performHealthCheck()`.
2. Start a debug session.
3. Inspect the task counters and health state.
4. Expand the three timer members.
5. Verify their configured periods.
6. Remove timing-related breakpoints before testing normal runtime behavior.

Breakpoints pause the processor and may distort task timing.

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
- Avoid blocking delays in the normal application loop.
- Give independent periodic activities independent timers.
- Keep cooperative tasks short and non-blocking.
- Monitor whether important tasks continue to make progress.
- Use rollover-safe unsigned time calculations.
- Pass a consistent timestamp through related operations.
- Encapsulate repeated hardware operations behind focused abstractions.
- Separate raw hardware input from confirmed application state.
- Represent one-time transitions as events.
- Keep hardware-independent algorithms testable without the board.
- Prefer composition over inheritance for application components.
- Prefer fixed-size and statically allocated resources.
- Avoid runtime dynamic allocation where practical.
- Keep interrupt handlers short.
- Add abstractions only when they improve clarity or testability.
- Validate communication input before processing commands.

## Planned Features

The firmware will be expanded to include:

- Hardware timer interrupts
- UART telemetry output
- UART command reception
- Command parsing and validation
- System operating modes
- Fault detection and fault management
- ADC sensor monitoring
- I2C sensor input
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
- Input model: Periodic polling with software debouncing
- Timing model: Non-blocking rollover-safe periodic timing
- Health model: Basic task-liveness monitoring
- RTOS: Not currently used
- Dynamic allocation: Avoided during normal firmware operation