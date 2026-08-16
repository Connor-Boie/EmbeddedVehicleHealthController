````markdown
# Embedded Vehicle Health Controller

A bare-metal embedded C++ prototype for the STM32 NUCLEO-F446RE that monitors vehicle-oriented system health, acquires redundant temperature measurements, detects and records runtime faults, persists diagnostic events in external SPI flash, supports diagnostic fault injection, recovers from application stalls through an independent watchdog, reports reset causes, processes serial commands, and transmits system diagnostics through UART.

The project uses STM32CubeMX-generated hardware initialization together with a separate application-owned C++ layer. Generated C code communicates with the C++ application through a small C-compatible bridge.

The current hardware is a bench prototype. Two colocated MCP9808 temperature sensors simulate redundant vehicle battery-temperature channels in a vehicle health-monitoring system.

## Current Features

- C++ application layer running above STM32 HAL initialization
- GPIO status heartbeat through a reusable `DigitalOutput` abstraction
- Automatic periodic system heartbeat
- Debounced USER button input
- USER button manual diagnostic/status snapshot
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
- Dual-channel temperature-health monitoring
- Redundant temperature averaging when both sensors agree
- Degraded single-sensor operation after one channel failure
- Sensor disagreement detection
- Overtemperature monitoring
- Temperature communication faults integrated with `FaultManager`
- External W25Q64 SPI NOR flash interface
- JEDEC flash identification
- SPI flash reads
- Page-aware SPI flash programming
- 4-KiB sector erase support
- Flash BUSY-status polling
- Destructive reserved-sector flash self-test
- Persistent structured diagnostic event logging
- Persistent startup/reset-cause records
- Persistent fault-activation records
- Persistent fault-clearing records
- Startup reconstruction of diagnostic-log state
- Flash-record checksum validation
- Detection of erased and invalid diagnostic records
- Monotonic diagnostic-event sequence numbering
- Explicit diagnostic-log erase command
- USART2 telemetry through the ST-LINK virtual COM port
- Interrupt-driven UART byte reception
- Fixed-capacity UART receive ring buffer
- Fixed-capacity UART line assembly without dynamic allocation
- UART command parsing and validation
- Immediate command acknowledgments and error responses
- Runtime diagnostic counters
- UART overflow, dropped-byte, and receive-error diagnostics

## Planned Features

- Persistent CAN communication and remote-node event history
- CAN communication between two STM32 nodes
- Second NUCLEO-F446RE thermal/actuator-control node
- Bidirectional CAN heartbeat supervision
- CAN communication-loss detection
- Board 2 thermal-control state machine
- PWM cooling output
- External LED warning indication
- Buzzer warning patterns
- Board 2 safe-state behavior
- Remote actuator/status feedback
- Remote-node fault propagation
- Host-side unit tests
- Automated build and test integration
- Final portfolio documentation and system diagrams

## Hardware

### Board 1

- STM32 NUCLEO-F446RE
- STM32F446RE microcontroller
- On-board LD2 status LED
- On-board USER push button
- ST-LINK USB virtual COM port
- Two MCP9808 temperature-sensor breakout boards
- W25Q64 64-Mbit / 8-MiB SPI NOR flash module
- Breadboard
- Male header pins
- Jumper wires

### Planned Board 2

- Second STM32 NUCLEO-F446RE
- CAN transceiver
- External warning LED or RGB LED
- Buzzer
- PWM-controlled cooling fan or simulated actuator
- USER button for actuator self-test or warning acknowledgement

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

The application stores normal 7-bit addresses. The MCP9808 driver shifts the address left by one when passing it to STM32 HAL I²C functions.

```text
Sensor A HAL address: 0x18 << 1 = 0x30
Sensor B HAL address: 0x19 << 1 = 0x32
```

## SPI Flash Configuration

The W25Q64 connects to SPI2.

The selected SPI2 alternate-function pins are exposed through the Nucleo ST Morpho headers.

```text
NUCLEO-F446RE                    W25Q64
------------------------------------------------
3V3                              VCC
GND                              GND
PB10 / CN10 pin 25 / SPI2_SCK    CLK / SCK
PC2  / CN7 pin 35 / SPI2_MISO    DO / MISO
PC1  / CN7 pin 36 / SPI2_MOSI    DI / MOSI
PB6  / FLASH_CS                  CS
```

`DI` and `DO` are named from the flash device's perspective:

```text
DI = flash data input  = STM32 MOSI
DO = flash data output = STM32 MISO
```

The Nucleo Arduino `D0`–`D15` headers and the ST Morpho headers are separate connector systems. CubeMX identifies pins using the STM32 GPIO names such as `PB10`, `PC2`, and `PC1`.

Chip select is controlled manually through PB6.

```text
CS HIGH → flash deselected
CS LOW  → flash selected
```

SPI2 uses:

```text
Master mode
Full duplex
8-bit data
MSB first
CPOL low
CPHA first edge
Software NSS
Baud-rate prescaler 16
SPI baud rate approximately 2.625 Mbit/s
```

The SPI clock is intentionally kept conservative for reliable breadboard and jumper-wire communication.

## Flash Organization

The current W25Q64 driver models:

```text
Total capacity: 8 MiB
Page size:      256 bytes
Sector size:    4096 bytes
```

Each memory address refers to one byte.

The 8-MiB device therefore provides byte addresses from:

```text
0x000000
through
0x7FFFFF
```

A 256-byte page contains 256 consecutive byte addresses.

A 4-KiB sector contains:

```text
4096 / 256 = 16 pages
```

Flash operations are separated into:

```text
READ
PROGRAM
ERASE
```

Programming can change erased bits from `1` toward `0`.

Returning programmed `0` bits to `1` requires an erase operation.

Erase operations occur at sector granularity, so erasing one sector affects all 4096 bytes in that sector.

`W25q64::program()` automatically splits writes that cross 256-byte page boundaries.

### Current Flash Memory Map

The first two sectors are reserved for persistent diagnostic logging.

```text
0x000000
┌──────────────────────────────┐
│ Diagnostic log sector 0      │
│ 0x000000 - 0x000FFF          │
├──────────────────────────────┤
│ Diagnostic log sector 1      │
│ 0x001000 - 0x001FFF          │
├──────────────────────────────┤
│                              │
│ Currently unused flash       │
│                              │
├──────────────────────────────┤
│ Reserved FLASH TEST sector   │
│ 0x7FF000 - 0x7FFFFF          │
└──────────────────────────────┘
0x7FFFFF
```

The diagnostic logger and destructive flash self-test therefore operate in separate regions.

## W25Q64 Driver

The `W25q64` C++ class provides:

- initialization and JEDEC identification
- arbitrary flash reads
- page-aware programming
- 4-KiB sector erase
- Write Enable handling
- Write Enable Latch verification
- Status Register 1 reads
- BUSY-bit polling
- address-range validation
- explicit chip-select control
- SPI communication failure tracking

### JEDEC Identification

The firmware sends the JEDEC ID command:

```text
0x9F
```

and reads three identification bytes.

The connected device currently reports:

```text
0xEF4017
```

indicating successful communication with the connected W25Q64 flash device.

### Page Programming

A single W25Q64 Page Program operation must remain within one 256-byte page.

If a requested write crosses a page boundary, the driver automatically splits it.

For example:

```text
Starting address: 250
Length:           20 bytes
```

is programmed as:

```text
Page 0:
6 bytes

Page 1:
14 bytes
```

Before each Page Program operation, the driver:

```text
Waits for flash to become ready
        ↓
Sends Write Enable
        ↓
Verifies Write Enable Latch
        ↓
Sends Page Program command and address
        ↓
Sends data
        ↓
Waits for programming to complete
```

### Sector Erase

Sector erase operates on 4096-byte boundaries.

If an address inside a sector is supplied, the driver rounds the address down to the beginning of that sector.

For example:

```text
Requested address:
0x001234

Containing sector:
0x001000 - 0x001FFF

Sector erase address:
0x001000
```

The erase command restores the entire sector to the erased state.

## Reserved Flash Test Sector

The final 4-KiB flash sector is reserved for development self-testing.

```text
Start address: 0x7FF000
End address:   0x7FFFFF
```

This region is not used by the persistent diagnostic logger.

The `FLASH TEST` command:

1. Refreshes the watchdog.
2. Erases the reserved sector.
3. Refreshes the watchdog.
4. Programs a known 32-byte pattern.
5. Refreshes the watchdog.
6. Reads the pattern back.
7. Compares every byte.
8. Reports success or failure.

The hardware flash self-test has been successfully verified using the connected W25Q64.

## Persistent Diagnostic Logging

`DiagnosticLogger` provides structured nonvolatile event storage using the existing `W25q64` driver.

The logger currently reserves two 4-KiB sectors:

```text
Start address: 0x000000
End address:   0x001FFF
Region size:   8192 bytes
```

The final W25Q64 sector at `0x7FF000` remains reserved exclusively for `FLASH TEST`.

### Diagnostic Record Format

Each diagnostic record occupies exactly 32 bytes.

```text
Offset   Size   Field
--------------------------------
0x00     4      magic
0x04     4      formatVersion
0x08     4      eventType
0x0C     4      sequence
0x10     4      uptimeMs
0x14     4      data0
0x18     4      data1
0x1C     4      checksum
--------------------------------
Total    32 bytes
```

The fixed-size record format provides deterministic storage without dynamic allocation.

Because:

```text
Page size   = 256 bytes
Record size = 32 bytes
```

each flash page contains:

```text
256 / 32 = 8 records
```

Each 4-KiB sector contains:

```text
4096 / 32 = 128 records
```

The two-sector logging region therefore provides:

```text
128 × 2 = 256 records
```

### Record Identification

Each valid record contains a fixed magic value and format version.

```text
Magic:          0x474C5645
Format version: 1
```

The magic value allows stored data to be distinguished from unrelated flash contents.

The format version provides a way for future firmware to recognize changes to the persistent record layout.

### Current Event Types

```text
1 = SystemStartup
2 = FaultActivated
3 = FaultCleared
```

The event-type field is stored as a fixed-width 32-bit value.

Future events can extend this enumeration for capabilities such as CAN communication failures and Board 2 actuator faults.

### System Startup Records

A startup event is written after the flash and diagnostic logger initialize successfully.

For `SystemStartup` records:

```text
data0 = primary ResetCause enum value
data1 = complete reset-cause mask
```

The primary reset cause represents one selected software interpretation.

The reset-cause mask preserves all reset flags observed by the reset-cause detector.

This allows a watchdog reset, power-on reset, software reset, or other supported cause to be retained across future resets and power cycles.

### Fault Transition Records

Persistent logging is transition-based rather than periodically writing the same active fault state.

When new fault bits become active:

```text
eventType = FaultActivated
data0     = fault bits that became active
data1     = complete active-fault mask afterward
```

When active fault bits clear:

```text
eventType = FaultCleared
data0     = fault bits that cleared
data1     = complete active-fault mask afterward
```

For example:

```text
data0 = 0x00000004
data1 = 0x00000004
```

for a `FaultActivated` event indicates that the Sensor A unavailable fault became active and remained the only active fault.

A later record containing:

```text
data0 = 0x00000004
data1 = 0x00000000
```

for a `FaultCleared` event indicates that the Sensor A unavailable fault cleared and no active faults remained.

Logging transitions instead of repeatedly logging steady-state faults reduces unnecessary flash writes.

### Sequence Numbers

Every successfully appended record receives a sequence number.

A normal sequence begins:

```text
0
1
2
3
...
```

The sequence number represents chronological event ordering rather than the physical flash slot number.

During startup, the logger scans existing valid records and reconstructs the next sequence number from persistent storage.

For example:

```text
log_last_sequence=5
log_next_sequence=6
```

means record sequence 5 is the most recent valid event and sequence 6 will be assigned to the next successfully appended event.

Sequence numbering therefore continues across MCU resets.

### Uptime Timestamp

Each event stores the current `HAL_GetTick()` value in:

```text
uptimeMs
```

This timestamp represents how long the current MCU boot had been running when that event was recorded.

The uptime value restarts after reset, while the persistent sequence number continues to provide ordering across multiple boots.

### Record Checksum

Each record contains a 32-bit checksum calculated from its stored fields.

The current implementation uses an FNV-1a-style checksum.

A record is accepted as valid only if:

```text
magic is correct
AND
format version is correct
AND
checksum matches
```

The checksum helps detect incomplete or corrupted flash records.

It is used as an integrity check for this bench prototype and is not presented as a safety-certified error-detection mechanism.

### Erased-State Detection

An erased NOR-flash byte reads as:

```text
0xFF
```

An unused 32-byte diagnostic-record slot therefore contains only erased `0xFF` values.

The logger considers a slot erased only when every 32-bit field contains:

```text
0xFFFFFFFF
```

Only completely erased slots are eligible for new records.

### Startup Log Recovery

`DiagnosticLogger::initialize()` scans all 256 record slots.

Each slot is classified as:

```text
ERASED
VALID
INVALID
```

An erased record can be used for a future append.

A valid record contains the correct magic value, record-format version, and checksum.

An invalid record contains programmed data but fails validation.

Invalid records are not overwritten because NOR flash cannot safely restore programmed `0` bits to `1` without erasing an entire sector.

The startup scan reconstructs:

- number of valid records
- number of invalid records
- latest valid record
- next sequence number
- next erased write location
- full/not-full state

This allows logging state to survive MCU resets without storing separate volatile metadata.

### Append Behavior

A new diagnostic event is appended by:

```text
Find next erased slot
        ↓
Construct 32-byte record
        ↓
Assign sequence number
        ↓
Calculate checksum
        ↓
Program record through W25q64
        ↓
Read record back
        ↓
Validate stored record
        ↓
Advance logger state
```

RAM bookkeeping is updated only after the programmed record has been read back and validated.

### Partially Written Records

If power is lost during a record program operation, the affected slot may contain partially programmed data.

Such a record will normally be:

```text
not erased
AND
not valid
```

During the next startup scan it is counted as an invalid record and is not reused.

The logger searches for a later completely erased slot instead.

This prevents a partially programmed NOR-flash region from being treated as safe writable storage.

### Log Full Behavior

The logger does not automatically erase old records when the two-sector logging region becomes full.

When all 256 record slots have been consumed:

```text
log_full=1
```

Further append attempts stop rather than silently destroying previous diagnostic history.

This intentionally favors preserving diagnostic evidence over automatic circular-buffer behavior.

A future implementation could add sector recycling or generation-based wear management if application requirements justify the additional complexity.

### Log Erase

The UART command:

```text
LOG ERASE
```

explicitly erases the two diagnostic-log sectors:

```text
0x000000 - 0x001FFF
```

A successful command responds:

```text
OK DIAGNOSTIC LOG ERASED
```

The reserved `FLASH TEST` sector is not affected.

After erasing:

```text
log_records=0
log_full=0
log_invalid_records=0
log_next_sequence=0
log_last_valid=0
```

A new startup record is created after the next reset.

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
│   │   ├── DiagnosticLogger.hpp
│   │   ├── DigitalOutput.hpp
│   │   ├── FaultInjector.hpp
│   │   ├── FaultManager.hpp
│   │   ├── Mcp9808.hpp
│   │   ├── PeriodicTimer.hpp
│   │   ├── ResetCauseDetector.hpp
│   │   ├── TemperatureHealthMonitor.hpp
│   │   ├── UartCommandReceiver.hpp
│   │   ├── UartTelemetry.hpp
│   │   ├── W25q64.hpp
│   │   ├── Watchdog.hpp
│   │   └── application_bridge.h
│   └── Src/
│       ├── Application.cpp
│       ├── ButtonDebouncer.cpp
│       ├── CommandParser.cpp
│       ├── DiagnosticLogger.cpp
│       ├── DigitalOutput.cpp
│       ├── FaultInjector.cpp
│       ├── FaultManager.cpp
│       ├── Mcp9808.cpp
│       ├── PeriodicTimer.cpp
│       ├── ResetCauseDetector.cpp
│       ├── TemperatureHealthMonitor.cpp
│       ├── UartCommandReceiver.cpp
│       ├── UartTelemetry.cpp
│       ├── W25q64.cpp
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

```text
                 MCP9808 A
                     │
                     ├── I²C ──┐
                     │          │
                 MCP9808 B      │
                                ▼
                     TemperatureHealthMonitor
                                │
                                ▼
                           FaultManager
                                │
               ┌────────────────┼──────────────────┐
               │                │                  │
               ▼                ▼                  ▼
             UART      DiagnosticLogger       Future CAN
                                │
                                ▼
                              W25Q64
```

The C++ application layer currently contains:

```text
Application
├── DigitalOutput
├── ButtonDebouncer
├── PeriodicTimer
├── FaultInjector
├── FaultManager
├── ResetCauseDetector
├── Mcp9808 Sensor A
├── Mcp9808 Sensor B
├── TemperatureHealthMonitor
├── W25q64
├── DiagnosticLogger
├── UartCommandReceiver
├── CommandParser
├── UartTelemetry
└── Watchdog
```

`Application` decides which system events are important enough to log.

`DiagnosticLogger` owns persistent record formatting, validation, sequencing, scanning, and append behavior.

`W25q64` remains responsible for the low-level SPI flash operations.

```text
Application
    │
    │ event policy
    ▼
DiagnosticLogger
    │
    │ persistent-record management
    ▼
W25q64
    │
    │ SPI flash operations
    ▼
W25Q64 hardware
```

## Cooperative Scheduling

Current task periods are:

```text
Button sampling:       5 ms
Heartbeat update:    500 ms
Health check:       1000 ms
Temperature sample: 1000 ms
Telemetry:          1000 ms
Watchdog refresh:    500 ms
```

The application uses `HAL_GetTick()` and reusable `PeriodicTimer` objects rather than blocking delays for normal periodic scheduling.

## Automatic Heartbeat

The on-board LD2 LED is an automatic system heartbeat.

Every 500 milliseconds:

```text
PeriodicTimer
      ↓
updateHeartbeat()
      ↓
LD2 toggles
      ↓
heartbeat_count increments
```

The heartbeat can no longer be manually disabled.

This makes the heartbeat a consistent indication that normal application scheduling is continuing.

The automatic heartbeat will later provide a natural foundation for CAN node-health supervision.

## USER Button

The Board 1 USER button is now a local diagnostic input.

A debounced press:

```text
increments button_presses
        ↓
immediately transmits current UART telemetry
```

The button no longer controls heartbeat state.

This behavior more closely represents a local status or diagnostic request.

The future Board 2 USER button is planned for actuator self-test or warning acknowledgement.

## MCP9808 Temperature Acquisition

Two `Mcp9808` objects provide independent redundant temperature channels.

```text
Sensor A: 0x18
Sensor B: 0x19
```

Each sensor tracks:

- availability
- most recent temperature
- successful reads
- communication failures

A failed channel does not prevent acquisition from the other channel.

## Temperature Health Monitoring

`TemperatureHealthMonitor` provides:

```text
REDUNDANT
DEGRADED_A
DEGRADED_B
DISAGREEMENT
UNAVAILABLE
```

### REDUNDANT

Both sensors are valid and agree within the configured threshold.

The selected temperature is their midpoint.

### DEGRADED_A

Only Sensor A is valid.

Sensor A remains usable as the selected temperature.

### DEGRADED_B

Only Sensor B is valid.

Sensor B remains usable as the selected temperature.

### DISAGREEMENT

Both sensors communicate but differ by more than the configured threshold.

The raw temperatures remain visible, but no trusted selected temperature is produced.

With only two sensors, the system cannot determine which sensor is incorrect.

### UNAVAILABLE

Neither sensor is valid.

No selected temperature is available.

## Temperature Thresholds

Current bench-prototype thresholds:

```text
Sensor disagreement: 2.000°C
Overtemperature:    60.000°C
```

These are demonstration thresholds rather than validated vehicle battery safety limits.

If either available sensor reports a temperature at or above the overtemperature threshold, the overtemperature fault becomes active even if the two sensors disagree.

## Independent Watchdog

The application refreshes the independent watchdog every 500 milliseconds during normal execution.

The `WATCHDOG TEST` command deliberately stops watchdog refreshes so hardware reset behavior can be verified.

The watchdog is configured for an approximately 2-second timeout using the STM32 independent low-speed clock.

Flash self-test operations refresh the watchdog between destructive flash operations so flash testing does not interfere with watchdog recovery behavior.

A watchdog-generated reboot is also captured by the persistent diagnostic logger as a `SystemStartup` record containing the detected reset-cause information.

## Reset-Cause Detection

Telemetry reports:

```text
reset_cause
reset_cause_mask
```

Supported causes include:

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

When both power-on and brownout flags are present during ordinary startup, power-on is given higher reporting priority.

The primary cause and complete cause mask are both persisted in `SystemStartup` diagnostic records.

## Fault Monitoring

Fault bits:

```text
Bit 0 — 0x00000001 — Button task timeout
Bit 1 — 0x00000002 — Hardware timer inactive
Bit 2 — 0x00000004 — Temperature Sensor A unavailable
Bit 3 — 0x00000008 — Temperature Sensor B unavailable
Bit 4 — 0x00000010 — Temperature sensor disagreement
Bit 5 — 0x00000020 — Overtemperature
```

Both active and latched masks are maintained.

A degraded temperature mode can still provide a valid selected temperature while the corresponding unavailable-sensor fault keeps the overall system health state faulted.

Changes in the active-fault mask are persisted as diagnostic records.

The logger stores only fault transitions rather than writing the same active state during every health-monitoring cycle.

## UART Telemetry

USART2 communicates through the ST-LINK virtual COM port.

```text
115200 baud
8 data bits
No parity
1 stop bit
No flow control
```

Current telemetry fields include:

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

temp_mode
temp_selected_valid
temp_selected_mC
temp_disagreement_mC

flash_available
flash_jedec_id
flash_failures
flash_test_run
flash_test_passed

log_initialized
log_records
log_capacity
log_full
log_invalid_records
log_failures
log_next_sequence
log_last_valid
log_last_type
log_last_sequence
log_last_uptime_ms
log_last_data0
log_last_data1

button_presses
heartbeat_count

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

Telemetry uses a fixed 1280-byte buffer and a bounded 150-ms UART transmission timeout.

### Diagnostic-Log Telemetry

Example:

```text
log_initialized=1
log_records=6
log_capacity=256
log_full=0
log_invalid_records=0
log_failures=0
log_next_sequence=6
log_last_valid=1
log_last_type=1
log_last_sequence=5
log_last_uptime_ms=65
log_last_data0=0x00000005
log_last_data1=0x00000014
```

The first group describes the logger itself:

```text
log_initialized       logger startup scan completed successfully
log_records           number of valid stored records
log_capacity          maximum number of record slots
log_full              no erased record slots remain
log_invalid_records   programmed records that failed validation
log_failures          logger read/write/verification failures
log_next_sequence     sequence assigned to the next successful event
```

The `log_last_*` fields describe the latest valid stored event:

```text
log_last_valid
log_last_type
log_last_sequence
log_last_uptime_ms
log_last_data0
log_last_data1
```

Current event-type values:

```text
1 = SystemStartup
2 = FaultActivated
3 = FaultCleared
```

The meanings of `data0` and `data1` depend on the event type.

For `SystemStartup`:

```text
data0 = primary ResetCause enum value
data1 = complete reset-cause mask
```

For `FaultActivated` and `FaultCleared`:

```text
data0 = fault bits that changed
data1 = active-fault mask after the transition
```

## UART Line Handling

Either character completes an input command:

```text
Carriage return: \r
Line feed:       \n
```

CR, LF, and CR+LF terminals are supported.

For CR+LF, the second terminator produces an empty line, which is ignored.

## UART Commands

### Status

```text
STATUS
```

### Fault Status

```text
FAULTS
```

### Reset Cause

```text
RESET CAUSE
```

### Temperatures

```text
TEMPERATURES
```

### Flash Status

```text
FLASH STATUS
```

Immediately sends telemetry containing flash availability, JEDEC ID, failure count, self-test state, and persistent diagnostic-log state.

### Flash Self-Test

```text
FLASH TEST
```

The final reserved 4-KiB flash sector is erased, programmed with a known pattern, read back, and verified.

A successful test responds:

```text
OK FLASH TEST PASSED
```

### Diagnostic Log Erase

```text
LOG ERASE
```

Erases only the two 4-KiB sectors reserved for persistent diagnostic records.

A successful erase responds:

```text
OK DIAGNOSTIC LOG ERASED
```

This command does not erase the final sector reserved for `FLASH TEST`.

### Fault Injection

```text
INJECT BUTTON FAULT
INJECT TIMER FAULT
CLEAR INJECTED FAULTS
```

Fault activation and clearing transitions generated through fault injection are also persisted by the diagnostic logger.

### Clear Counters

```text
CLEAR
```

### Clear Latched Faults

```text
CLEAR FAULTS
```

Clearing the RAM-based latched-fault mask does not erase persistent diagnostic history.

### Watchdog Test

```text
WATCHDOG TEST
```

After the watchdog resets the processor, the next startup record preserves the watchdog reset cause in external flash.

### Invalid Commands

Unrecognized commands receive:

```text
ERROR INVALID COMMAND
```

The previous:

```text
HEARTBEAT ON
HEARTBEAT OFF
```

commands have been removed because heartbeat operation is now automatic.

## SPI Flash Verification

### Startup Identification

Successful startup communication produces:

```text
flash_available=1
flash_jedec_id=0xEF4017
flash_failures=0
```

This confirms successful SPI communication and flash identification.

When persistent logger initialization also succeeds:

```text
log_initialized=1
```

is reported.

### Self-Test

Run:

```text
FLASH TEST
```

Expected response:

```text
OK FLASH TEST PASSED
```

followed by telemetry containing:

```text
flash_test_run=1
flash_test_passed=1
```

A passing self-test verifies:

```text
SPI2 configuration
SCK communication
MOSI communication
MISO communication
chip-select control
JEDEC identification
status-register polling
Write Enable
sector erase
page programming
flash reads
byte-for-byte verification
```

The self-test operates only on `0x7FF000–0x7FFFFF`, so existing persistent diagnostic records are preserved.

### Persistent Log Verification

A clean diagnostic-log test can begin with:

```text
LOG ERASE
```

Expected logger state:

```text
log_records=0
log_full=0
log_invalid_records=0
log_next_sequence=0
log_last_valid=0
```

After resetting the board once, a startup record should be present:

```text
log_records=1
log_last_type=1
log_last_sequence=0
log_next_sequence=1
```

Resetting again should retain the previous record and append another startup record:

```text
log_records=2
log_last_sequence=1
log_next_sequence=2
```

This verifies that the log survives MCU reset and reconstructs sequence state from external flash.

A fault-transition test can use:

```text
INJECT BUTTON FAULT
```

After the next health check, a `FaultActivated` event should be appended.

Leaving the fault active should not continuously create additional records.

After:

```text
CLEAR INJECTED FAULTS
```

a `FaultCleared` event should be appended when the health monitor confirms the transition.

### Watchdog Persistence Verification

Run:

```text
WATCHDOG TEST
```

The board should reset through the independent watchdog.

After reboot:

```text
reset_cause=INDEPENDENT_WATCHDOG
```

should be reported and another `SystemStartup` event should appear in the persistent log.

Previous diagnostic records should remain intact.

### Flash Disconnection

With the board powered off, disconnect the flash module and restart.

Expected:

```text
flash_available=0
```

The rest of the controller should continue operating.

Persistent diagnostic logging will be unavailable while the flash device is unavailable.

External flash unavailability currently does not force the controller into a fatal state.

## Current Verified Telemetry Example

A successful system state can resemble:

```text
reset_cause=POWER_ON
temp_a_available=1
temp_b_available=1
temp_mode=REDUNDANT
temp_selected_valid=1

flash_available=1
flash_jedec_id=0xEF4017
flash_failures=0

log_initialized=1
log_capacity=256
log_full=0
log_invalid_records=0
log_failures=0

healthy=1
timer_active=1
active_faults=0x00000000
watchdog_failures=0
```

The W25Q64 erase/program/read self-test has been successfully verified on hardware.

Persistent records have also been observed surviving controller resets with sequence state reconstructed during startup.

## Future Two-Node Architecture

The current controller will become Board 1 of a distributed CAN system.

```text
BOARD 1 — Vehicle Health Controller
        │
        ├── redundant temperature sensing
        ├── fault management
        ├── watchdog recovery
        ├── persistent SPI diagnostic logging
        ├── UART diagnostics
        └── CAN
             │
             │
           CAN bus
             │
             ▼
BOARD 2 — Thermal / Actuator Controller
        │
        ├── CAN supervision
        ├── thermal-control state machine
        ├── PWM cooling output
        ├── external warning LED
        ├── buzzer
        ├── safe-state handling
        ├── actuator status feedback
        └── watchdog
```

Board 1 will transmit system-health and temperature information over CAN.

Board 2 will use that information to determine the required thermal-control response.

Example future thermal states may include:

```text
NORMAL
WARM
COOLING
HIGH
CRITICAL
SAFE
```

Cooling output will increase gradually with temperature using PWM rather than operating only as a simple on/off output.

Board 2 will also provide visible and audible warning behavior through an external LED and buzzer.

Both nodes will supervise CAN heartbeat messages.

Loss of communication will cause the affected node to report a communication fault and transition to defined safe behavior.

Board 2 will transmit its own health, actuator status, and fault information back to Board 1.

Board 1 will eventually store important remote-node events in the existing persistent SPI diagnostic log.

Potential future persistent events include:

- CAN communication lost
- CAN communication restored
- Board 2 startup/reset
- remote actuator fault
- thermal safe-state entry
- critical thermal condition
- Board 2 heartbeat loss

## Design Principles

- Separate generated hardware code from application-owned C++
- Keep hardware configuration in the `.ioc` file
- Keep interrupt handlers short
- Avoid dynamic allocation
- Use fixed-capacity buffers
- Use fixed-width integer types
- Use bounded blocking operations
- Keep redundant sensor channels independent
- Separate raw sensor acquisition from health decisions
- Separate active faults from latched fault history
- Keep heartbeat behavior automatic and deterministic
- Use the USER button for diagnostics rather than disabling system-health behavior
- Keep SPI chip selection explicit
- Validate external flash identity during startup
- Poll flash BUSY state before dependent operations
- Perform Write Enable before destructive flash operations
- Handle flash page boundaries in the driver
- Reserve a dedicated development sector for destructive testing
- Keep persistent diagnostic storage separate from the destructive test sector
- Use fixed-size diagnostic records
- Include record-format metadata for persistent data
- Validate persistent records before trusting them
- Detect completely erased flash before appending
- Do not overwrite partially programmed or invalid NOR-flash records
- Read back and verify newly programmed diagnostic records
- Log fault transitions rather than repeatedly logging steady-state faults
- Reconstruct persistent logger state after reset
- Preserve existing diagnostic history when the logging region becomes full
- Require an explicit command before erasing persistent diagnostic history
- Verify programmed flash data by reading it back
- Continue operating if an external peripheral is unavailable
- Preserve watchdog recovery behavior during peripheral operations
- Prepare subsystem boundaries for future CAN integration
- Use defined safe-state behavior for future distributed-node failures
````
