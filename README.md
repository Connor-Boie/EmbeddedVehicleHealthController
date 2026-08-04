# Embedded Vehicle Health Controller

A bare-metal embedded C++ prototype for the STM32 NUCLEO-F446RE that monitors vehicle-oriented system health, acquires redundant temperature measurements, detects and records runtime faults, supports diagnostic fault injection, recovers from application stalls through an independent watchdog, reports reset causes, processes serial commands, and transmits system diagnostics through UART.

The project uses STM32CubeMX-generated hardware initialization together with a separate application-owned C++ layer. Generated C code communicates with the C++ application through a small C-compatible bridge.

The current hardware is a bench prototype. Two colocated MCP9808 temperature sensors simulate redundant vehicle battery-temperature channels in a vehicle health-monitoring system.

## Current Features

- C++ application layer running above STM32 HAL initialization
- GPIO status LED control through a reusable `DigitalOutput` abstraction
- Debounced USER button input
- Button-controlled heartbeat enable and disable behavior
- Cooperative periodic task scheduling
- TIM7 hardware timer interrupts
- Main-loop processing of interrupt-generated timer events
- Runtime task and timer health monitoring
- Bit-mask-based active fault tracking
- Latched historical fault tracking
- Controlled diagnostic fault injection
- Independent hardware watchdog protection
- Controlled watchdog-reset testing through UART
- Reset-cause detection and reporting
- Dual MCP9808 temperature acquisition over I²C
- Independent temperature-sensor availability tracking
- Independent sensor read and communication-failure counters
- USART2 telemetry through the ST-LINK virtual COM port
- Interrupt-driven UART byte reception
- Fixed-capacity UART receive ring buffer
- Fixed-capacity UART line assembly without dynamic allocation
- UART command parsing and validation
- Immediate command acknowledgments and error responses
- Runtime counters for commands, watchdog refreshes, sensor reads, and communication failures
- UART overflow, dropped-byte, and receive-error diagnostics

## Planned Features

- MCP9808 communication faults integrated with `FaultManager`
- Redundant temperature-channel selection
- Degraded operation when only one temperature sensor is available
- Temperature disagreement detection
- Overtemperature fault detection
- Temperature and sensor fault injection
- SPI flash storage for persistent diagnostic event records
- Persistent reset, fault, and temperature-event history
- CAN communication for live system-health and temperature messages
- Host-side unit tests
- Automated build and test integration
- Final portfolio documentation and system diagrams

## Hardware

- STM32 NUCLEO-F446RE
- STM32F446RE microcontroller
- On-board LD2 status LED
- On-board USER push button
- ST-LINK USB virtual COM port
- Two MCP9808 temperature-sensor breakout boards
- Breadboard
- Male header pins
- Jumper wires

## Temperature-Sensor Configuration

Both MCP9808 sensors share the same I²C bus:

```text
NUCLEO-F446RE      Sensor A       Sensor B
------------------------------------------------
3V3                VDD / VIN      VDD / VIN
GND                GND            GND
D15 / SCL / PB8    SCL            SCL
D14 / SDA / PB9    SDA            SDA
```

The sensors use different address-pin configurations.

### Sensor A

```text
A0 → GND
A1 → GND
A2 → GND

7-bit I²C address: 0x18
```

### Sensor B

```text
A0 → 3V3
A1 → GND
A2 → GND

7-bit I²C address: 0x19
```

The application stores the normal 7-bit addresses. The MCP9808 driver shifts each address left by one when passing it to STM32 HAL I²C functions.

```text
Sensor A HAL address: 0x18 << 1 = 0x30
Sensor B HAL address: 0x19 << 1 = 0x32
```

The sensors remain logically identified as `0x18` and `0x19`.

## Development Tools

- STM32CubeMX
- STM32CubeIDE
- GNU Arm Embedded Toolchain
- Git
- PuTTY or another serial terminal

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
│   │   ├── Mcp9808.hpp
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
│       ├── Mcp9808.cpp
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

The project separates generated hardware startup code from application-owned C++ logic.

```text
STM32CubeMX-generated C code
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
            ├── Mcp9808 Sensor A
            ├── Mcp9808 Sensor B
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
- I2C1
- TIM7
- The independent watchdog
- The on-board LED
- The USER button

Application calls are placed inside STM32 `USER CODE` regions so CubeMX regeneration does not overwrite them.

### C/C++ Bridge

`application_bridge.h` provides C-compatible functions callable from generated C code.

`application_bridge.cpp` owns the C++ `Application` instance and forwards bridge calls to it.

The bridge preserves CubeMX-generated `main.c` while allowing the application to be implemented in C++.

## Cooperative Scheduling

The application uses `HAL_GetTick()` and reusable `PeriodicTimer` objects to schedule work without blocking delays.

Current task periods are:

```text
Button sampling:       5 ms
Heartbeat update:    500 ms
Health check:       1000 ms
Temperature sample: 1000 ms
Telemetry:          1000 ms
Watchdog refresh:    500 ms
```

Each call to `Application::run()` checks which tasks are due and executes them cooperatively.

The temperature task runs before telemetry when both timers expire during the same loop iteration. This allows the telemetry message to contain the newest available measurements.

The watchdog refresh occurs near the end of the application-loop path. A stall earlier in the loop therefore prevents watchdog servicing.

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

## MCP9808 Temperature Acquisition

Two `Mcp9808` objects represent independent redundant temperature channels.

```text
Sensor A address: 0x18
Sensor B address: 0x19
```

Both sensors share I2C1 while maintaining separate:

- Availability state
- Most recent temperature
- Successful-read counter
- Failure counter
- Device identity validation
- Address configuration

### Sensor Initialization

During application startup, each sensor is checked independently.

Initialization performs:

1. I²C device-ready check
2. Manufacturer-ID register read
3. Manufacturer-ID validation
4. Device-ID register read
5. Device-ID validation
6. Initial temperature read

A missing or invalid sensor does not stop the entire controller. The unavailable channel is reported through telemetry while the remaining firmware continues operating.

### Temperature Representation

Temperatures are stored as signed integer milli-degrees Celsius.

Examples:

```text
25000 m°C = 25.000°C
24625 m°C = 24.625°C
-5000 m°C = -5.000°C
```

Using scaled integers avoids requiring floating-point formatting in embedded telemetry.

### Independent Acquisition

Each sensor is read separately.

A failure on Sensor A does not prevent an attempt to read Sensor B.

This structure supports future degraded operation in which the controller continues using one valid temperature channel when the other channel fails.

### Current Limitation

The current version acquires and reports both measurements but does not yet determine:

- Whether one sensor is inaccurate
- Whether the channels disagree beyond an allowed threshold
- Which valid channel should be selected as the system temperature
- Whether a temperature is above an overtemperature threshold

Those health-monitoring decisions are planned for a later feature.

## Independent Watchdog

The STM32 independent watchdog protects against complete application stalls.

Configuration:

```text
Prescaler:       64
Reload counter:  999
Approx. timeout: 2 seconds
```

The application normally refreshes the watchdog every 500 milliseconds.

If application-loop progress stops, watchdog refreshes stop and the MCU resets after the configured timeout.

Telemetry reports:

```text
watchdog_refresh_enabled
watchdog_refreshes
watchdog_failures
```

The `WATCHDOG TEST` command deliberately disables application watchdog refreshes so hardware recovery can be tested.

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

The readable primary cause selects the most diagnostically useful result, while the complete mask preserves every captured flag.

Power-on reset is given priority over brownout when both flags appear during a normal power-up sequence.

Example:

```text
reset_cause=INDEPENDENT_WATCHDOG
reset_cause_mask=0x00000010
```

## Fault Monitoring

`FaultManager` tracks failures through active and latched 32-bit masks.

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

Temperature-sensor fault bits are not yet implemented.

## Diagnostic Fault Injection

`FaultInjector` allows existing faults to be simulated without stopping tasks or changing hardware configuration.

```text
Fault active = real failure OR diagnostic injection
```

Clearing an injection allows the active fault to recover during the next health check. Latched history remains until `CLEAR FAULTS`.

Current injectable faults:

```text
Button task timeout
Hardware timer inactive
```

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

temp_a_available
temp_a_mC
temp_a_reads
temp_a_failures

temp_b_available
temp_b_mC
temp_b_reads
temp_b_failures

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

Example temperature fields:

```text
temp_a_available=1
temp_a_mC=24750
temp_a_reads=12
temp_a_failures=0
```

This represents:

```text
Sensor A available:   yes
Temperature:          24.750°C
Successful reads:     12
Communication errors: 0
```

A sensor with:

```text
temp_b_available=0
```

should not have its stored temperature treated as a current valid measurement.

Telemetry uses a fixed-size 768-byte character buffer and a bounded UART timeout.

No dynamic allocation is used.

## Interrupt-Driven UART Reception

USART2 reception is started with `HAL_UART_Receive_IT()`.

One byte is received at a time.

The receive-complete callback forwards the byte to a fixed-capacity ring buffer and rearms reception.

Command parsing and application decisions occur in main-loop context rather than interrupt context.

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

Immediately transmits the complete current telemetry line.

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

### Temperatures

```text
TEMPERATURES
```

Immediately transmits telemetry containing both MCP9808 temperature channels, availability states, read counters, and failure counters.

### Heartbeat Control

```text
HEARTBEAT ON
HEARTBEAT OFF
```

Responses:

```text
OK HEARTBEAT ON
OK HEARTBEAT OFF
```

### Fault Injection

```text
INJECT BUTTON FAULT
INJECT TIMER FAULT
CLEAR INJECTED FAULTS
```

Responses:

```text
OK BUTTON FAULT INJECTED
OK TIMER FAULT INJECTED
OK INJECTED FAULTS CLEARED
```

### Clear Counters

```text
CLEAR
```

Response:

```text
OK COUNTERS CLEARED
```

This clears application-owned operational counters such as command, button, heartbeat, and health-check counts.

Peripheral-driver diagnostic counters may remain separate.

### Clear Latched Faults

```text
CLEAR FAULTS
```

Response:

```text
OK FAULTS CLEARED
```

A fault that remains active will become latched again during health monitoring.

### Watchdog Reset Test

```text
WATCHDOG TEST
```

Response:

```text
OK WATCHDOG RESET EXPECTED
```

The application stops refreshing the independent watchdog.

After the watchdog resets the board, telemetry should report:

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
2. Open `EmbeddedVehicleHealthController.ioc`.
3. Confirm I2C1 uses PB8 for SCL and PB9 for SDA.
4. Confirm USART2, TIM7, GPIO, and IWDG remain configured.
5. Generate code when hardware configuration changes.
6. Verify application calls remain inside STM32 `USER CODE` regions.
7. Refresh the project.
8. Clean the project.
9. Build the project.
10. Connect the MCP9808 sensors while the board is unpowered.
11. Connect the Nucleo board through the ST-LINK USB connector.
12. Program the board using the Run or Debug configuration.
13. Open the ST-LINK virtual COM port at 115200 8-N-1 with no flow control.

Only one program can open the COM port at a time.

## Basic Hardware Test

With both sensors connected:

```text
Sensor A address: 0x18
Sensor B address: 0x19
```

Expected telemetry:

```text
temp_a_available=1
temp_b_available=1
```

Both read counters should increase approximately once per second.

To test a partial hardware failure:

1. Power off the board.
2. Disconnect Sensor B.
3. Power the board back on.
4. Observe telemetry.

Expected behavior:

```text
temp_a_available=1
temp_b_available=0
```

Sensor A should continue updating.

The heartbeat, UART commands, timer interrupt, telemetry, and watchdog should remain operational.

## Design Principles

- Separate generated hardware code from application-owned code
- Keep CubeMX hardware configuration in the `.ioc` file
- Keep application modifications inside protected `USER CODE` regions
- Preserve generated C startup code through a C-compatible bridge
- Keep interrupt handlers short
- Perform parsing and application decisions in main-loop context
- Avoid dynamic allocation
- Use fixed-capacity buffers
- Use fixed-width integer types
- Use scaled integers for physical measurements
- Use bounded blocking operations
- Keep hardware-specific dependencies near the application boundary
- Represent commands, faults, and reset causes with strongly typed enumerations
- Keep redundant sensor channels independent
- Attempt each sensor transaction independently
- Preserve the last successful measurement for diagnostics
- Mark stale measurements invalid through an explicit availability field
- Continue operating when one optional peripheral is unavailable
- Validate sensor identity rather than relying only on an address acknowledgment
- Preserve intermittent failures with latched fault history
- Preserve all captured reset flags while selecting one readable primary cause
- Clear hardware reset flags only after copying them into application state
- Keep test injection separate from production fault state
- Refresh the watchdog only after application-loop progress
- Allow hardware recovery from complete firmware stalls
- Track failures rather than silently ignoring them