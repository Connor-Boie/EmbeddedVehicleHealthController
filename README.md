# Embedded Vehicle Health Controller

A bare-metal embedded C++ application for the STM32 NUCLEO-F446RE that monitors system activity, controls a status heartbeat, detects and latches runtime faults, processes user input, and exposes diagnostics through UART.

The project uses STM32CubeMX-generated hardware initialization together with a separate C++ application layer. Generated C code communicates with the C++ application through a small C-compatible bridge.

## Current Features

- C++ application layer running above STM32 HAL initialization
- GPIO status LED control through a reusable `DigitalOutput` abstraction
- Debounced USER button input
- Button-controlled heartbeat enable and disable behavior
- Cooperative periodic task scheduling
- TIM7 hardware timer interrupts
- Main-loop processing of interrupt-generated timer events
- Runtime health monitoring
- Bit-mask-based active fault tracking
- Latched historical fault tracking
- USART2 telemetry through the ST-LINK virtual COM port
- Interrupt-driven UART byte reception
- Fixed-capacity UART receive ring buffer
- Fixed-capacity UART line assembly without dynamic allocation
- UART command parsing and validation
- Immediate command acknowledgments and error responses
- Runtime counters for valid and invalid commands
- UART overflow, dropped-byte, and receive-error diagnostics

## Hardware

- STM32 NUCLEO-F446RE
- STM32F446RE microcontroller
- On-board LD2 status LED
- On-board USER push button
- ST-LINK USB virtual COM port

## Development Tools

- STM32CubeMX
- STM32CubeIDE
- GNU Arm Embedded Toolchain
- Git

## Project Structure

```text
EmbeddedVehicleHealthController/
├── App/
│   ├── Inc/
│   │   ├── Application.hpp
│   │   ├── ButtonDebouncer.hpp
│   │   ├── CommandParser.hpp
│   │   ├── DigitalOutput.hpp
│   │   ├── FaultManager.hpp
│   │   ├── PeriodicTimer.hpp
│   │   ├── UartCommandReceiver.hpp
│   │   ├── UartTelemetry.hpp
│   │   └── application_bridge.h
│   └── Src/
│       ├── Application.cpp
│       ├── ButtonDebouncer.cpp
│       ├── CommandParser.cpp
│       ├── DigitalOutput.cpp
│       ├── FaultManager.cpp
│       ├── PeriodicTimer.cpp
│       ├── UartCommandReceiver.cpp
│       ├── UartTelemetry.cpp
│       └── application_bridge.cpp
├── Core/
│   ├── Inc/
│   └── Src/
│       └── main.c
├── Drivers/
├── EmbeddedVehicleHealthController.ioc
└── README.md
```

STM32CubeIDE metadata and generated build-output directories are omitted from this overview.

## Architecture

The project separates generated hardware startup code from application-owned C++ code.

```text
STM32 generated C code
        │
        ▼
C-compatible application bridge
        │
        ▼
C++ Application object
        │
        ├── DigitalOutput
        ├── ButtonDebouncer
        ├── PeriodicTimer
        ├── FaultManager
        ├── UartCommandReceiver
        ├── CommandParser
        └── UartTelemetry
```

### Generated C Layer

STM32CubeMX generates peripheral configuration and startup code in `Core/Src/main.c`.

The generated layer initializes:

- The system clock
- GPIO
- USART2
- TIM7
- The on-board LED
- The USER button

Application calls are placed inside STM32 `USER CODE` regions so CubeMX regeneration does not overwrite them.

### C/C++ Bridge

`application_bridge.h` provides C-compatible functions that can be called from generated C code.

`application_bridge.cpp` owns the C++ `Application` instance and forwards bridge calls to it.

The bridge allows the project to preserve CubeMX-generated `main.c` while implementing application behavior in C++.

## Cooperative Scheduling

The application uses `HAL_GetTick()` and reusable `PeriodicTimer` objects to schedule work without blocking delays.

Current task periods are:

```text
Button sampling:     5 ms
Heartbeat update:  500 ms
Health check:     1000 ms
Telemetry:        1000 ms
```

Each call to `Application::run()` checks which tasks are due and runs them cooperatively.

No task intentionally waits for another scheduled task to finish.

## GPIO Behavior

### Status LED

The on-board LD2 LED is controlled through the `DigitalOutput` class.

The LED toggles every 500 milliseconds while heartbeat behavior is enabled.

When heartbeat behavior is disabled, the LED is immediately forced off.

### USER Button

The USER button is read through the STM32 board-support package.

A `ButtonDebouncer` object filters mechanical transitions before creating a press event.

Each accepted button press:

- Toggles heartbeat enable state
- Increments the button press counter
- Turns the LED off when heartbeat behavior becomes disabled

## Hardware Timer Interrupt

TIM7 generates a periodic hardware interrupt.

The interrupt callback performs only minimal work:

```text
TIM7 interrupt
    │
    ▼
HAL timer callback
    │
    ▼
C-compatible bridge
    │
    ▼
Increment timer interrupt counter
```

Application-level processing occurs later in the main loop.

This keeps the interrupt service path short and avoids performing complex work inside interrupt context.

The health monitor verifies that the TIM7 interrupt counter continues changing.

## Fault Monitoring

The `FaultManager` tracks failures using 32-bit masks.

Two masks are maintained:

```text
Active fault mask
Latched fault mask
```

### Active Faults

An active fault represents a failure that is occurring during the current health check.

When the monitored condition recovers, its active fault bit is cleared.

### Latched Faults

A latched fault records that a failure has occurred.

Its bit remains set after the active condition recovers, preserving evidence of intermittent failures.

The `CLEAR FAULTS` command clears resolved historical faults. A fault that is still active remains latched.

### Current Fault Bits

```text
Bit 0 — 0x00000001 — Button task timeout
Bit 1 — 0x00000002 — Hardware timer inactive
```

Common mask values:

```text
0x00000000 — No faults
0x00000001 — Button task timeout
0x00000002 — Hardware timer inactive
0x00000003 — Both faults
```

Overall system health is derived from the active fault mask.

```text
No active faults  → healthy=1
Active fault      → healthy=0
```

## UART Telemetry

USART2 is configured for asynchronous serial communication through the ST-LINK virtual COM port.

Serial settings:

```text
Baud rate:    115200
Data bits:    8
Parity:       None
Stop bits:    1
Flow control: None
```

Periodic telemetry includes fields such as:

```text
uptime_ms
button_presses
heartbeat_enabled
healthy
timer_active
timer_irq_count
rx_lines
valid_commands
invalid_commands
active_faults
latched_faults
rx_dropped_bytes
rx_overflow_lines
rx_errors
```

Example:

```text
uptime_ms=12000 button_presses=1 heartbeat_enabled=1 healthy=1 timer_active=1 timer_irq_count=120 rx_lines=3 valid_commands=2 invalid_commands=1 active_faults=0x00000000 latched_faults=0x00000002 rx_dropped_bytes=0 rx_overflow_lines=0 rx_errors=0
```

Telemetry uses a fixed-size character buffer and bounded UART transmission timeout.

No dynamic allocation is used.

## Interrupt-Driven UART Reception

USART2 reception is started with `HAL_UART_Receive_IT()`.

One byte is received at a time.

The receive-complete callback:

1. Forwards the received byte to the C++ application.
2. Places the byte into a fixed-capacity ring buffer.
3. Rearms interrupt-driven reception for the next byte.

The interrupt does not parse commands or assemble complete lines.

Those operations occur in the main application loop.

## UART Ring Buffer

`UartCommandReceiver` uses a single-producer, single-consumer ring buffer.

The interrupt callback is the producer:

```text
UART interrupt → write byte into ring buffer
```

The application loop is the consumer:

```text
Application loop → remove and process byte
```

The buffer has fixed capacity and does not allocate memory dynamically.

When the ring buffer is full, the incoming byte is dropped and `rx_dropped_bytes` increases.

## UART Line Handling

Received bytes are assembled into null-terminated command lines.

Either of the following characters completes a line:

```text
Carriage return: \r
Line feed:       \n
```

This supports terminals that send:

```text
CR
LF
CR+LF
```

For a `CR+LF` sequence, the first terminator completes the command and the second attempts to complete an empty line. Empty lines are ignored.

The maximum command length is 63 characters, excluding the null terminator.

A line that exceeds the fixed capacity is discarded and `rx_overflow_lines` increases.

## UART Commands

Commands are uppercase and must match exactly.

### Request Status

```text
STATUS
```

Immediately transmits the current status telemetry.

### Request Fault Status

```text
FAULTS
```

Immediately transmits telemetry containing the active and latched fault masks.

### Enable Heartbeat

```text
HEARTBEAT ON
```

Enables heartbeat LED behavior.

Response:

```text
OK HEARTBEAT ON
```

### Disable Heartbeat

```text
HEARTBEAT OFF
```

Disables heartbeat behavior and immediately turns the status LED off.

Response:

```text
OK HEARTBEAT OFF
```

### Clear Application Counters

```text
CLEAR
```

Resets software-level application counters, including:

- Received line count
- Valid command count
- Invalid command count
- Button press count
- Heartbeat execution count
- Health check count

Response:

```text
OK COUNTERS CLEARED
```

Hardware timer, fault, and low-level UART diagnostic counters are intentionally not reset.

### Clear Latched Faults

```text
CLEAR FAULTS
```

Clears historical faults that are no longer active.

A currently active fault remains latched until the underlying condition recovers.

Response:

```text
OK FAULTS CLEARED
```

### Invalid Commands

Unrecognized commands receive:

```text
ERROR INVALID COMMAND
```

Invalid commands increase the `invalid_commands` counter.

Command matching is currently case-sensitive. For example, `STATUS` is valid while `status` is invalid.

## Health Monitoring

The application currently checks:

- Whether the button-sampling task ran within its expected time window
- Whether TIM7 hardware interrupts continue occurring

Each failed check sets a separate active and latched fault bit.

The system reports healthy only when the active fault mask is zero.

Telemetry fields:

```text
healthy=1
timer_active=1
active_faults=0x00000000
```

indicate that the monitored application task and hardware timer are operating as expected.

## Build and Run

1. Open the project in STM32CubeIDE.
2. Generate or update peripheral configuration through the project `.ioc` file.
3. Build the project.
4. Connect the NUCLEO board through the ST-LINK USB connector.
5. Program the board using the STM32CubeIDE Run or Debug configuration.
6. Open the ST-LINK virtual COM port in a serial terminal using 115200 8-N-1 with no flow control.

Only one program can open the COM port at a time.

## Design Principles

- Separate generated hardware code from application-owned code
- Keep interrupt handlers short
- Perform parsing and application decisions in main-loop context
- Avoid dynamic allocation
- Use fixed-capacity buffers
- Use bounded blocking operations
- Keep hardware-specific dependencies near the application boundary
- Represent commands and faults with strongly typed enumerations
- Represent multiple faults efficiently with bit masks
- Preserve intermittent failures with latched fault history
- Track failures and overflow conditions rather than silently ignoring them