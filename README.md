# Embedded Vehicle Health Controller

A bare-metal embedded C++ prototype for the STM32 NUCLEO-F446RE that monitors vehicle-oriented system health, controls a status heartbeat, detects and latches runtime faults, supports controlled fault injection, recovers from application stalls through an independent watchdog, reports reset causes, processes user commands, and exposes diagnostics through UART.

The project uses STM32CubeMX-generated hardware initialization together with a separate C++ application layer. Generated C code communicates with the C++ application through a small C-compatible bridge.

The current hardware is a bench prototype. Planned temperature sensing represents redundant vehicle battery-temperature monitoring rather than installation in an operating vehicle.

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
- Controlled diagnostic fault injection
- Fault recovery and latching verification through UART commands
- Independent hardware watchdog protection
- Controlled watchdog-reset testing through UART
- Reset-cause detection and reporting
- USART2 telemetry through the ST-LINK virtual COM port
- Interrupt-driven UART byte reception
- Fixed-capacity UART receive ring buffer
- Fixed-capacity UART line assembly without dynamic allocation
- UART command parsing and validation
- Immediate command acknowledgments and error responses
- Runtime counters for commands, watchdog refreshes, and communication failures
- UART overflow, dropped-byte, and receive-error diagnostics

## Planned Hardware Integration

- Two MCP9808 sensors on one I²C bus for redundant vehicle battery-temperature monitoring
- Sensor communication, overtemperature, and disagreement fault detection
- SPI flash storage for persistent diagnostic event records
- CAN communication for live health and temperature messages between nodes

## Hardware

- STM32 NUCLEO-F446RE
- STM32F446RE microcontroller
- On-board LD2 status LED
- On-board USER push button
- ST-LINK USB virtual COM port
- Two MCP9808 temperature-sensor breakout boards planned for I²C integration

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
│   │   ├── FaultInjector.hpp
│   │   ├── FaultManager.hpp
│   │   ├── PeriodicTimer.hpp
│   │   ├── ResetCauseDetector.hpp
│   │   ├── UartCommandReceiver.hpp
│   │   ├── UartTelemetry.hpp
│   │   ├── Watchdog.hpp
│   │   └── application_bridge.h
│   └── Src/
│       ├── Application.cpp
│       ├── ButtonDebouncer.cpp
│       ├── CommandParser.cpp
│       ├── DigitalOutput.cpp
│       ├── FaultInjector.cpp
│       ├── FaultManager.cpp
│       ├── PeriodicTimer.cpp
│       ├── ResetCauseDetector.cpp
│       ├── UartCommandReceiver.cpp
│       ├── UartTelemetry.cpp
│       ├── Watchdog.cpp
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
        ├── FaultInjector
        ├── FaultManager
        ├── ResetCauseDetector
        ├── UartCommandReceiver
        ├── CommandParser
        ├── UartTelemetry
        └── Watchdog
```

### Generated C Layer

STM32CubeMX generates peripheral configuration and startup code in `Core/Src/main.c`.

The generated layer initializes:

- The system clock
- GPIO
- USART2
- TIM7
- The independent watchdog
- The on-board LED
- The USER button

Application calls are placed inside STM32 `USER CODE` regions so CubeMX regeneration does not overwrite them.

### C/C++ Bridge

`application_bridge.h` provides C-compatible functions callable from generated C code.

`application_bridge.cpp` owns the C++ `Application` instance and forwards bridge calls to it.

The bridge preserves CubeMX-generated `main.c` while implementing application behavior in C++.

## Cooperative Scheduling

The application uses `HAL_GetTick()` and reusable `PeriodicTimer` objects to schedule work without blocking delays.

Current task periods are:

```text
Button sampling:       5 ms
Heartbeat update:    500 ms
Health check:       1000 ms
Telemetry:          1000 ms
Watchdog refresh:    500 ms
```

Each call to `Application::run()` checks which tasks are due and runs them cooperatively.

The watchdog refresh occurs near the end of the application-loop path so a stall earlier in the loop prevents watchdog servicing.

## GPIO Behavior

### Status LED

The on-board LD2 LED is controlled through the `DigitalOutput` class.

The LED toggles every 500 milliseconds while heartbeat behavior is enabled.

When heartbeat behavior is disabled, the LED is immediately forced off.

### USER Button

The USER button is read through the STM32 board-support package.

A `ButtonDebouncer` filters mechanical transitions before creating a press event.

Each accepted press:

- Toggles heartbeat enable state
- Increments the button press counter
- Turns the LED off when heartbeat becomes disabled

## Hardware Timer Interrupt

TIM7 generates a periodic hardware interrupt.

The interrupt callback performs minimal work:

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

The health monitor verifies that the TIM7 interrupt counter continues changing.

## Independent Watchdog

The STM32 independent watchdog protects against complete application stalls.

Configuration:

```text
Prescaler:       64
Reload counter:  999
Approx. timeout: 2 seconds
```

The application refreshes the watchdog every 500 milliseconds during normal operation.

If application-loop progress stops, watchdog refreshes stop and the MCU resets after the timeout.

Telemetry reports:

```text
watchdog_refresh_enabled
watchdog_refreshes
watchdog_failures
```

The `WATCHDOG TEST` command deliberately disables application refreshes so hardware recovery can be tested.

After reset, normal startup enables watchdog servicing again.

## Reset-Cause Detection

`ResetCauseDetector` reads STM32 RCC reset flags during application initialization.

The flags are copied into application-owned state before the hardware flags are cleared.

Telemetry reports:

```text
reset_cause
reset_cause_mask
```

Supported primary causes:

```text
POWER_ON
BROWNOUT
EXTERNAL_PIN
SOFTWARE
INDEPENDENT_WATCHDOG
WINDOW_WATCHDOG
LOW_POWER
UNKNOWN
```

Application mask bits:

```text
Bit 0 — 0x00000001 — Power-on reset
Bit 1 — 0x00000002 — Brownout reset
Bit 2 — 0x00000004 — External reset pin
Bit 3 — 0x00000008 — Software reset
Bit 4 — 0x00000010 — Independent watchdog reset
Bit 5 — 0x00000020 — Window watchdog reset
Bit 6 — 0x00000040 — Low-power reset
```

Multiple hardware flags may be present after one startup.

The readable primary cause selects the most diagnostically useful cause, while the complete mask preserves every captured flag.

Example:

```text
reset_cause=INDEPENDENT_WATCHDOG reset_cause_mask=0x00000010
```

## Fault Monitoring

`FaultManager` tracks failures using active and latched 32-bit masks.

### Active Faults

An active fault represents a failure occurring during the current health check.

When the condition recovers, its active bit clears.

### Latched Faults

A latched fault records that a failure occurred.

Its bit remains set after recovery, preserving evidence of intermittent failures.

`CLEAR FAULTS` clears resolved historical faults. A currently active fault remains latched.

### Current Fault Bits

```text
Bit 0 — 0x00000001 — Button task timeout
Bit 1 — 0x00000002 — Hardware timer inactive
```

## Diagnostic Fault Injection

`FaultInjector` allows existing faults to be simulated without stopping tasks or changing hardware configuration.

```text
Fault active = real failure OR diagnostic injection
```

Clearing an injection allows the active fault to recover during the next health check. Latched history remains until `CLEAR FAULTS`.

## UART Telemetry

USART2 communicates through the ST-LINK virtual COM port.

Serial settings:

```text
Baud rate:    115200
Data bits:    8
Parity:       None
Stop bits:    1
Flow control: None
```

Periodic telemetry includes:

```text
uptime_ms
reset_cause
reset_cause_mask
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
injected_faults
watchdog_refresh_enabled
watchdog_refreshes
watchdog_failures
rx_dropped_bytes
rx_overflow_lines
rx_errors
```

Telemetry uses a fixed-size 512-byte character buffer and a bounded UART timeout.

No dynamic allocation is used.

## Interrupt-Driven UART Reception

USART2 reception is started with `HAL_UART_Receive_IT()`.

One byte is received at a time.

The receive-complete callback forwards the byte to a fixed-capacity ring buffer and rearms reception.

Command parsing and application decisions occur in main-loop context.

## UART Line Handling

Either character completes an input line:

```text
Carriage return: \r
Line feed:       \n
```

This supports CR, LF, and CR+LF terminals.

For CR+LF, the first terminator completes the command and the second produces an empty line, which is ignored.

The maximum command length is 63 characters, excluding the null terminator.

## UART Commands

Commands are uppercase and must match exactly.

### Status

```text
STATUS
```

Immediately transmits current telemetry.

### Fault Status

```text
FAULTS
```

Immediately transmits telemetry containing active, latched, and injected fault masks.

### Reset Cause

```text
RESET CAUSE
```

Immediately transmits telemetry containing the reset-cause name and mask.

### Heartbeat Control

```text
HEARTBEAT ON
HEARTBEAT OFF
```

### Fault Injection

```text
INJECT BUTTON FAULT
INJECT TIMER FAULT
CLEAR INJECTED FAULTS
```

### Clear Counters

```text
CLEAR
```

### Clear Latched Faults

```text
CLEAR FAULTS
```

### Watchdog Reset Test

```text
WATCHDOG TEST
```

Response:

```text
OK WATCHDOG RESET EXPECTED
```

The application stops refreshing the IWDG. After the watchdog reset, telemetry should report:

```text
reset_cause=INDEPENDENT_WATCHDOG
```

### Invalid Commands

Unrecognized commands receive:

```text
ERROR INVALID COMMAND
```

Command matching is case-sensitive.

## Build and Run

1. Open the project in STM32CubeIDE.
2. Configure peripherals through the project `.ioc` file.
3. Generate code when hardware configuration changes.
4. Refresh and build the project.
5. Connect the NUCLEO board through the ST-LINK USB connector.
6. Program the board using the Run or Debug configuration.
7. Open the ST-LINK virtual COM port at 115200 8-N-1 with no flow control.

Only one program can open the COM port at a time.

## Design Principles

- Separate generated hardware code from application-owned code
- Keep interrupt handlers short
- Perform parsing and application decisions in main-loop context
- Avoid dynamic allocation
- Use fixed-capacity buffers
- Use bounded blocking operations
- Keep hardware-specific dependencies near the application boundary
- Represent commands, faults, and reset causes with strongly typed enumerations
- Preserve intermittent failures with latched fault history
- Preserve all captured reset flags while selecting one readable primary cause
- Clear hardware reset flags only after copying them into application state
- Keep test injection separate from production fault state
- Refresh the watchdog only after application-loop progress
- Allow hardware recovery from complete firmware stalls
- Track failures rather than silently ignoring them