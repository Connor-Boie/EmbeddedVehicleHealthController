# Embedded Vehicle Health Controller

A bare-metal embedded C++ firmware project for the
STMicroelectronics NUCLEO-F446RE development board.

The project simulates a vehicle subsystem controller that collects sensor
data, tracks system state, detects and reports faults, receives commands,
and transmits telemetry.

## Project Status

Board bring-up, mixed C/C++ integration, GPIO output control, debounced
button input, cooperative scheduling, TIM7 hardware interrupts, health
monitoring, and USART2 telemetry output are complete.

Current capabilities include:

- STM32F446RE initialization generated through STM32CubeMX
- Mixed C and C++ firmware
- A user-owned C++ application layer
- C-compatible application and interrupt bridges
- A reusable digital-output abstraction
- Hardware-independent button debouncing
- Rollover-safe periodic timing
- Multiple cooperative periodic tasks
- A configurable status heartbeat
- TIM7 periodic hardware interrupts
- Task and hardware-event liveness monitoring
- Fixed-buffer USART2 telemetry formatting
- Serial telemetry through the ST-LINK virtual COM port
- Successful and failed telemetry transmission counters
- No runtime dynamic allocation in application code

The application currently runs four cooperative periodic activities:

```text
Button sampling task    every 5 ms
Heartbeat task          every 500 ms
Health-monitoring task  every 1000 ms
Telemetry task          every 1000 ms
```

TIM7 independently generates an interrupt approximately every:

```text
100 milliseconds
```

## Target Hardware

- Development board: NUCLEO-F446RE
- Microcontroller: STM32F446RE
- Processor core: Arm Cortex-M4
- Onboard debugger/programmer: ST-LINK
- Onboard user LED: LD2 on PA5
- Onboard user button: B1 on PC13
- Hardware timer: TIM7
- Telemetry interface: USART2
- USART2 transmit pin: PA2
- USART2 receive pin: PA3

## Development Tools

- STM32CubeMX
- STM32CubeIDE
- STM32CubeF4 firmware package
- Arm GNU toolchain
- Git
- Serial terminal software

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
3. Generated GPIO, TIM7, USART2, and board-support initialization runs.
4. PA5 is initialized as an output for LD2.
5. PA2 and PA3 are initialized for USART2.
6. The USER button is initialized through the board-support library.
7. Generated C code initializes the C++ application through a bridge.
8. The application initializes its components, timers, counters, and health
   state.
9. TIM7 is started with interrupt generation enabled.
10. The main loop repeatedly calls `Application::run()`.
11. The button task samples and debounces the USER button.
12. The heartbeat task toggles LD2 while enabled.
13. The health task evaluates button-task and TIM7 activity.
14. TIM7 interrupts increment an interrupt-side counter.
15. The main loop observes hardware-timer events.
16. The telemetry task formats controller state into a fixed-size buffer.
17. A structured status line is transmitted through USART2 once per second.
18. The ST-LINK virtual COM port carries the USART2 data to the connected PC.

## Telemetry Output

USART2 is configured as:

```text
Mode:          Asynchronous
Baud rate:     115200
Word length:   8 bits
Parity:        None
Stop bits:     1
Flow control:  None
TX pin:        PA2
RX pin:        PA3
```

The telemetry task runs every:

```text
1000 milliseconds
```

Example output:

```text
uptime_ms=5000 button_presses=2 heartbeat_enabled=1 healthy=1 timer_active=1 timer_irq_count=50
```

Fields include:

```text
uptime_ms
    Milliseconds since HAL startup.

button_presses
    Number of confirmed USER-button presses.

heartbeat_enabled
    1 when heartbeat blinking is enabled, otherwise 0.

healthy
    1 when the monitored application conditions are healthy.

timer_active
    1 when TIM7 produced interrupts during the latest health interval.

timer_irq_count
    Total number of observed TIM7 interrupts.
```

Messages end with:

```text
\r\n
```

to support conventional serial-terminal line endings.

## UART Telemetry Architecture

Telemetry behavior is implemented in:

```text
App/Inc/UartTelemetry.hpp
App/Src/UartTelemetry.cpp
```

The execution flow is:

```text
Application telemetry timer
    |
    v
Application::sendTelemetry()
    |
    v
UartTelemetry::sendStatus()
    |
    +--> Format into fixed-size buffer
    |
    +--> Validate formatted length
    |
    +--> HAL_UART_Transmit()
    |
    v
USART2
    |
    v
ST-LINK virtual COM port
    |
    v
PC serial terminal
```

`UartTelemetry` uses an internal fixed-size character buffer:

```cpp
char buffer_[192]{};
```

No `std::string`, heap allocation, `new`, or `malloc` is required.

The class tracks:

```text
messageCount_
    Successful telemetry transmissions.

failureCount_
    Formatting or UART transmission failures.
```

## Blocking Transmission Tradeoff

The current telemetry implementation uses:

```cpp
HAL_UART_Transmit()
```

This is a polling-mode operation.

The call uses a bounded timeout of:

```text
50 milliseconds
```

The current message normally transmits faster than this timeout, but the
cooperative main loop cannot run other tasks during the transmission.

Future revisions may use interrupt-driven or DMA transmission with explicit
buffer ownership and busy-state management.

## Software Architecture

The generated STM32 entry point remains in C:

```text
Core/Src/main.c
```

User-owned application behavior is implemented in:

```text
App/Inc/Application.hpp
App/Src/Application.cpp
```

The C-compatible bridge is implemented in:

```text
App/Inc/application_bridge.h
App/Src/application_bridge.cpp
```

Reusable components include:

```text
App/Inc/DigitalOutput.hpp
App/Src/DigitalOutput.cpp

App/Inc/ButtonDebouncer.hpp
App/Src/ButtonDebouncer.cpp

App/Inc/PeriodicTimer.hpp
App/Src/PeriodicTimer.cpp

App/Inc/UartTelemetry.hpp
App/Src/UartTelemetry.cpp
```

## Repository Structure

The diagram highlights the primary source, configuration, and documentation
files. Generated build output and IDE metadata are omitted for clarity.

```text
EmbeddedVehicleHealthController/
├── App/
│   ├── Inc/
│   │   ├── Application.hpp
│   │   ├── ButtonDebouncer.hpp
│   │   ├── DigitalOutput.hpp
│   │   ├── PeriodicTimer.hpp
│   │   ├── UartTelemetry.hpp
│   │   └── application_bridge.h
│   └── Src/
│       ├── Application.cpp
│       ├── ButtonDebouncer.cpp
│       ├── DigitalOutput.cpp
│       ├── PeriodicTimer.cpp
│       ├── UartTelemetry.cpp
│       └── application_bridge.cpp
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   └── stm32f4xx_it.h
│   └── Src/
│       ├── main.c
│       ├── stm32f4xx_hal_msp.c
│       └── stm32f4xx_it.c
├── Drivers/
│   ├── CMSIS/
│   └── STM32F4xx_HAL_Driver/
├── EmbeddedVehicleHealthController.ioc
├── STM32F446RETX_FLASH.ld
├── STM32F446RETX_RAM.ld
├── startup_stm32f446retx.s
├── .gitignore
└── README.md
```

The repository may also contain STM32CubeIDE metadata such as:

```text
.project
.cproject
.settings/
.mxproject
```

These are omitted from the architecture diagram.

## Cooperative Tasks

### Button task

Period:

```text
5 milliseconds
```

Responsibilities:

- Read the raw USER-button state
- Update software debouncing
- Generate confirmed button events
- Enable or disable heartbeat operation
- Record button-task execution time
- Count confirmed button presses

### Heartbeat task

Period:

```text
500 milliseconds
```

Responsibilities:

- Check whether heartbeat operation is enabled
- Toggle LD2
- Count heartbeat executions

### Health-monitoring task

Period:

```text
1000 milliseconds
```

Responsibilities:

- Check button-task freshness
- Check TIM7 interrupt activity
- Update hardware-timer activity
- Update overall system health
- Count health evaluations

### Telemetry task

Period:

```text
1000 milliseconds
```

Responsibilities:

- Capture the current controller state
- Format a bounded status message
- Transmit the message through USART2
- Record successful transmissions
- Record formatting or transmission failures

The health task executes before telemetry when both are due, allowing the
telemetry message to report the latest health result.

## TIM7 Hardware Interrupt

TIM7 uses:

```text
APB1 timer clock      84 MHz
Prescaler             8399
Counter period        999
Update period         approximately 100 ms
```

The interrupt path is:

```text
TIM7 update event
    |
    v
TIM7_IRQHandler()
    |
    v
HAL_TIM_IRQHandler()
    |
    v
HAL_TIM_PeriodElapsedCallback()
    |
    v
application_timer_interrupt()
    |
    v
Application::onTimerInterrupt()
```

Interrupt context only increments a counter. Larger operations remain in the
main loop.

## Generated-Code Source of Truth

The `.ioc` file is the source of truth for hardware configuration.

Hardware settings that must remain configured include:

```text
PA5        GPIO_Output
PA2        USART2_TX
PA3        USART2_RX
TIM7       Activated
USART2     Asynchronous
TIM7 IRQ   Enabled
```

Manual changes to generated hardware configuration may be removed during the
next code-generation operation.

## C and C++ Integration

STM32CubeMX generates hardware initialization and the firmware entry point in
C.

The application and reusable components are implemented in C++.

The bridge functions use C-compatible linkage:

```cpp
extern "C" void application_init(void);
extern "C" void application_run(void);
extern "C" void application_timer_interrupt(void);
```

The C++ application accesses the generated C UART handle through a
C-linkage declaration:

```cpp
extern "C"
{
extern UART_HandleTypeDef huart2;
}
```

This declaration refers to the existing generated handle and does not create
a second UART object.

## Build and Run

1. Open the `.ioc` file in STM32CubeMX.
2. Confirm PA5 is configured as `GPIO_Output`.
3. Confirm USART2 is configured as asynchronous.
4. Confirm PA2 and PA3 are assigned to USART2.
5. Confirm USART2 uses 115200 baud, 8 data bits, no parity, and one stop bit.
6. Confirm TIM7 configuration remains intact.
7. Generate code.
8. Build the project in STM32CubeIDE.
9. Connect the board through the ST-LINK USB connector.
10. Find the ST-LINK virtual COM port in Windows Device Manager.
11. Open that port in a serial terminal.
12. Configure the terminal for 115200 8-N-1 with no flow control.
13. Run the firmware.
14. Confirm one telemetry line appears approximately every second.
15. Press the USER button and confirm the reported button count and heartbeat
    state change.
16. Confirm `healthy` and `timer_active` normally remain `1`.

## Debugging

Useful telemetry values include:

```text
telemetry_.buffer_
telemetry_.messageCount_
telemetry_.failureCount_
```

Application-level accessors include:

```cpp
telemetryMessageCount()
telemetryFailureCount()
```

Set a breakpoint in:

```cpp
Application::sendTelemetry()
```

or:

```cpp
UartTelemetry::sendStatus()
```

Inspect:

```text
formattedLength
buffer_
transmitStatus
messageCount_
failureCount_
```

Remove timing-related breakpoints before judging normal periodic behavior.

## Generated-Code Policy

Manual changes to generated files must be placed only inside protected
regions:

```c
/* USER CODE BEGIN ... */

/* USER CODE END ... */
```

Application behavior belongs in user-owned files under `App/`.

Hardware configuration changes belong in the `.ioc` file.

## Design Principles

- Keep generated hardware initialization separate from application logic.
- Treat the `.ioc` file as the hardware-configuration source of truth.
- Keep the generated entry point small.
- Implement application behavior in user-owned C++ classes.
- Keep cooperative tasks short and bounded.
- Avoid indefinite waits.
- Use fixed-size buffers for embedded communication.
- Validate formatted lengths before transmission.
- Avoid runtime dynamic allocation.
- Keep interrupt handlers minimal.
- Process substantial asynchronous work in the main loop.
- Monitor task and hardware-event liveness.
- Use rollover-safe time calculations.
- Separate raw hardware inputs from confirmed application state.
- Represent one-time state transitions as events.
- Prefer composition for application components.
- Add abstractions when they improve ownership, clarity, or testability.

## Planned Features

The firmware will be expanded to include:

- UART command reception
- Interrupt-driven receive handling
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
- Telemetry interface: USART2 through ST-LINK virtual COM
- Telemetry format: Fixed-buffer structured text
- UART transmit model: Bounded polling transmission
- Timing model: Non-blocking periodic scheduling with bounded UART transmit
- Interrupt model: Minimal ISR notification with main-loop processing
- Hardware timer: TIM7 periodic update interrupt
- Health model: Task and hardware-event liveness monitoring
- RTOS: Not currently used
- Dynamic allocation: Avoided during normal firmware operation