# Embedded Vehicle Health Controller

A bare-metal embedded C++ prototype for the STM32 NUCLEO-F446RE that models a distributed vehicle-health and thermal-control system across two STM32 nodes.

Board 1 monitors vehicle-oriented system health, acquires redundant temperature measurements, detects and records runtime faults, persists diagnostic events in external SPI flash, supports diagnostic fault injection, recovers from application stalls through an independent watchdog, reports reset causes, processes serial commands, and prepares periodic vehicle-health status frames for CAN transmission.

Board 2 provides the foundation for a remote thermal/actuator-control node. It initializes CAN in normal mode, receives and decodes the shared Vehicle Health Status message format, and reports decoded remote status through USART2.

The project uses STM32CubeMX-generated hardware initialization together with separate application-owned C++ layers. Generated C code communicates with each board's C++ application through a small C-compatible bridge.

The current hardware is a bench prototype. Two colocated MCP9808 temperature sensors on Board 1 simulate redundant vehicle battery-temperature channels. Internal CAN loopback has been verified on Board 1. The two-node normal-mode CAN software framework builds successfully for both boards; physical CAN communication is awaiting final validation with replacement transceiver hardware.

## Current Features

### Shared System Architecture

- Separate STM32CubeIDE/CubeMX firmware projects for Board 1 and Board 2
- Shared CAN protocol definitions under `Shared/Inc`
- Fixed-width CAN message fields and explicit byte layout
- Separate generated STM32 code from application-owned C++
- C-compatible bridge between generated C startup code and C++ applications
- No dynamic allocation in the application design
- Independent buildable firmware targets for both nodes

### Board 1 — Vehicle Health Controller

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
- Persistent fault-activation and fault-clearing records
- Diagnostic-log reconstruction after reset
- Diagnostic-record checksum validation
- CAN1 bxCAN peripheral configuration
- 500-kbit/s CAN bit timing
- Standard 11-bit CAN frame transmission and reception
- CAN receive-filter configuration
- Verified internal CAN loopback
- Vehicle-health CAN frame serialization
- CAN1 normal-mode configuration for physical two-node communication
- Periodic Vehicle Health Status transmission framework
- Manual `CAN TEST` transmit command
- USART2 telemetry through the ST-LINK virtual COM port
- Interrupt-driven UART byte reception
- Fixed-capacity UART receive ring buffer
- Fixed-capacity UART line assembly without dynamic allocation
- UART command parsing and validation
- Immediate command acknowledgments and error responses
- Runtime diagnostic counters
- UART overflow, dropped-byte, and receive-error diagnostics

### Board 2 — Thermal / Actuator Controller

- Separate STM32 NUCLEO-F446RE firmware project
- C++ application layer above STM32 HAL initialization
- C-compatible bridge from generated `main.c` to the C++ application
- CAN1 normal-mode initialization
- Matching 500-kbit/s CAN bit timing
- Standard 11-bit CAN receive support
- Receive FIFO 0 polling
- Shared `0x100` Vehicle Health Status protocol decoding
- Protocol-version validation
- Payload-length validation
- Status-flag decoding
- Signed little-endian temperature decoding
- 32-bit little-endian remote fault-mask decoding
- USART2 startup diagnostics
- USART2 reporting of decoded remote vehicle-health data
- Stored remote vehicle-health state on Board 2
- Board 2 CAN communication states: `WAITING_FOR_DATA`, `CONNECTED`, and `COMMUNICATION_LOST`
- 1500-ms remote CAN communication timeout supervision
- Wraparound-safe elapsed-time checking with `HAL_GetTick()`
- Software-only remote-status self-test using a synthetic CAN frame
- UART reporting when the remote communication state changes
- Thermal states: `NORMAL`, `WARM`, `COOLING`, `HIGH`, `CRITICAL`, and `SAFE`
- Thermal decisions based on trusted remote temperature plus communication freshness
- Safe-state selection when remote temperature is invalid or communication is unavailable
- Software-only thermal-state self-test covering all thermal states without physical CAN hardware
- UART reporting when the thermal-control state changes

## Current CAN Validation Status

Board 1 CAN1 internal loopback has been verified successfully. That test demonstrated the STM32 bxCAN controller, receive filter, transmit mailbox path, receive FIFO path, standard identifier handling, payload serialization, and byte-for-byte receive verification without requiring an external transceiver.

Both Board 1 and Board 2 now build with CAN1 configured in normal mode at 500 kbit/s. Board 1 can queue the shared Vehicle Health Status frame, and Board 2 initializes CAN successfully and waits for matching traffic.

Physical two-node CAN communication has not yet been declared verified. Initial external transceiver modules did not produce a valid differential dominant bus state during bench diagnostics, so replacement transceiver hardware will be used before physical-bus validation is completed.

This distinction keeps software validation separate from physical-layer validation.

## Planned Features

- Physical CAN communication validation between the two STM32 nodes
- Persistent CAN communication and remote-node event history
- Bidirectional CAN heartbeat supervision
- Physical PWM cooling output
- Physical RGB LED warning indication
- Physical passive-buzzer tone generation and output
- Board 2 watchdog supervision
- USER button actuator self-test or warning acknowledgement
- Board 2 actuator/status frame transmitted back to Board 1
- Remote actuator/status feedback
- Remote-node fault propagation
- Board 1 remote-node communication supervision
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
- CAN transceiver interface
- Breadboard
- Male header pins
- Jumper wires

### Board 2

- STM32 NUCLEO-F446RE
- STM32F446RE microcontroller
- ST-LINK USB virtual COM port
- CAN transceiver interface
- On-board USER push button
- External warning LED or RGB LED planned
- Buzzer planned
- PWM-controlled cooling fan or simulated actuator planned

## Repository Structure

```text
EmbeddedVehicleHealthController/
├── Board1_VehicleHealthController/
│   ├── App/
│   │   ├── Inc/
│   │   └── Src/
│   ├── Core/
│   │   ├── Inc/
│   │   └── Src/
│   ├── Drivers/
│   └── EmbeddedVehicleHealthController.ioc
├── Board2_ThermalActuatorController/
│   ├── App/
│   │   ├── Inc/
│   │   └── Src/
│   ├── Core/
│   │   ├── Inc/
│   │   └── Src/
│   ├── Drivers/
│   └── Board2_ThermalActuatorController.ioc
├── Shared/
│   └── Inc/
│       └── CanProtocol.hpp
├── .gitignore
└── README.md
```

STM32CubeIDE metadata and generated build-output directories are omitted from this overview.

## System Architecture

```text
BOARD 1 — Vehicle Health Controller
        │
        ├── MCP9808 Sensor A ─┐
        ├── MCP9808 Sensor B ─┴─ I²C
        │
        ├── TemperatureHealthMonitor
        ├── FaultManager
        ├── Independent Watchdog
        ├── W25Q64 Diagnostic Logger ─ SPI
        ├── UART Diagnostics
        │
        └── CAN1
             │
             │  Vehicle Health Status
             │  Standard ID 0x100
             ▼
         CAN TRANSCEIVER
             │
           CANH/CANL
             │
         CAN TRANSCEIVER
             ▼
           CAN1
             │
BOARD 2 — Thermal / Actuator Controller
        │
        ├── Vehicle Health Status decoder
        ├── UART diagnostics
        ├── remote communication supervision
        ├── thermal-control state machine
        ├── actuator-command policy
        │    ├── target cooling duty
        │    ├── RGB warning color
        │    └── buzzer warning pattern
        ├── passive-buzzer pattern sequencer
        ├── physical PWM cooling output planned
        ├── physical RGB LED output planned
        ├── physical passive-buzzer output planned
        └── actuator status feedback planned
```

## Shared CAN Protocol

CAN message definitions shared by both firmware projects are kept in:

```text
Shared/Inc/CanProtocol.hpp
```

The first message is the Board 1 Vehicle Health Status frame.

```text
Standard CAN ID: 0x100
Payload length:  8 bytes
Protocol version: 1
```

Payload layout:

```text
Byte 0       Protocol version
Byte 1       Status flags
Bytes 2-3    Selected temperature in 0.1°C, signed 16-bit little-endian
Bytes 4-7    Active fault mask, unsigned 32-bit little-endian
```

Status-flag bits:

```text
Bit 0 = system healthy
Bit 1 = selected temperature valid
Bit 2 = Sensor A available
Bit 3 = Sensor B available
```

When no trusted selected temperature is available, the encoded temperature uses the signed 16-bit sentinel value:

```text
-32768
0x8000
```

Using a shared protocol header prevents Board 1 and Board 2 from independently redefining message identifiers, byte indexes, status bits, or sentinel values.

## CAN1 Configuration

Both nodes use CAN1 in normal mode for the physical two-node design.

```text
CAN peripheral clock:    42 MHz
Prescaler:               6
Bit Segment 1:           11 TQ
Bit Segment 2:           2 TQ
Synchronization Jump:    1 TQ
Bit rate:                500 kbit/s
```

The total CAN bit time is:

```text
1 synchronization TQ + 11 BS1 TQ + 2 BS2 TQ = 14 TQ
```

Therefore:

```text
42,000,000 / (6 × 14)
= 500,000 bits/second
```

CAN1 uses:

```text
PA11 = CAN1_RX
PA12 = CAN1_TX
```

The current receive filter accepts all CAN identifiers during bring-up and assigns matching traffic to receive FIFO 0. The filter can be narrowed after the distributed message set is finalized.

## Board 1 CAN Transmission

Board 1 builds a current Vehicle Health Status frame from its application state.

The frame includes:

```text
protocol version
system-health state
selected-temperature validity
Sensor A availability
Sensor B availability
selected temperature
active fault mask
```

Temperature is converted from the application's internal millidegree-Celsius representation to signed deci-degrees Celsius before serialization.

For example:

```text
24.7°C
→ 247 deci-degrees Celsius
```

The signed 16-bit value is serialized little-endian.

The active fault mask is serialized as four little-endian bytes.

The intended normal runtime behavior is periodic transmission every:

```text
500 ms
```

The UART command:

```text
CAN TEST
```

also requests an immediate Vehicle Health Status transmission.

A successful queue operation reports:

```text
OK CAN FRAME QUEUED
```

This response means the application successfully handed the frame to the bxCAN transmit path. It does not by itself prove that another physical node received or acknowledged the frame.

## Board 2 CAN Reception

Board 2 continuously polls receive FIFO 0.

When an available frame has:

```text
ID = 0x100
```

Board 2:

1. validates the payload length,
2. validates the protocol version,
3. decodes the status flags,
4. reconstructs the signed little-endian temperature,
5. reconstructs the 32-bit little-endian fault mask,
6. increments the Vehicle Health Status receive counter,
7. reports the decoded values through USART2.

A decoded message can resemble:

```text
can_rx_count=1 protocol=1 remote_healthy=1 remote_temp_valid=1 remote_sensor_a=1 remote_sensor_b=1 remote_temp_dC=247 remote_fault_mask=0x00000000
```

`remote_temp_dC=247` represents:

```text
24.7°C
```

## Board 2 Thermal-Control State Machine

Board 2 converts the latest trusted remote temperature into a higher-level thermal-control state.

```text
NORMAL
WARM
COOLING
HIGH
CRITICAL
SAFE
```

Current bench-prototype thresholds are:

```text
NORMAL    below 35.0°C
WARM      35.0°C to below 45.0°C
COOLING   45.0°C to below 55.0°C
HIGH      55.0°C to below 60.0°C
CRITICAL  60.0°C and above
SAFE      communication unavailable/lost or selected temperature invalid
```

The state machine intentionally does not require the remote `systemHealthy` flag to be true before using a valid selected temperature. Board 1 can be globally faulted while still operating in a valid degraded single-sensor temperature mode.

Board 2 therefore treats communication freshness and selected-temperature validity as the gating conditions for thermal decisions.

The current state machine is software-only and does not yet command a physical fan, LED, or buzzer. A synthetic self-test validates every state without requiring CAN transceiver hardware.

## Board 2 Actuator-Command Policy

Board 2 now converts each thermal-control state into a software-only actuator command. This layer remains independent of GPIO, timers, PWM channels, MOSFET hardware, the RGB LED, and the passive buzzer.

Current command mapping:

```text
Thermal state   Cooling duty   RGB warning   Buzzer pattern
NORMAL          0%             GREEN         OFF
WARM            0%             YELLOW        OFF
COOLING         40%            BLUE          OFF
HIGH            70%            ORANGE        SLOW_BEEP
CRITICAL        100%           RED           FAST_BEEP
SAFE            100%           MAGENTA       FAULT
```

`SAFE` requests full cooling because stale or unavailable temperature data should not silently disable cooling. The physical fan is not driven in this checkpoint; these percentages are target commands only.

The `40%` and `70%` cooling values are initial control-policy targets, not yet validated fan operating points. When the fan and MOSFET are connected, the minimum reliable startup duty and useful PWM range will be measured and the policy can be tuned if needed.

A synthetic actuator-command self-test verifies every mapping without requiring the physical CAN transceivers, fan, MOSFET, RGB LED, or buzzer.

## Board 2 Passive-Buzzer Pattern Timing

Board 2 now converts the high-level buzzer command into a non-blocking software timing signal. The timing layer does not yet generate an audible tone; it only decides whether a future tone-generation PWM output should currently be enabled.

Current software timing:

```text
OFF         always inactive

SLOW_BEEP   250 ms active
            750 ms inactive
            1000 ms total period

FAST_BEEP   200 ms active
            200 ms inactive
            400 ms total period

FAULT       150 ms active
            150 ms inactive
            150 ms active
            1050 ms inactive
            1500 ms total period
```

The `FAULT` pattern is intentionally a distinct double beep rather than a faster version of the temperature warning.

Pattern changes restart the new timing sequence immediately. The implementation uses unsigned elapsed-time subtraction so the timing remains correct across the `HAL_GetTick()` 32-bit rollover.

A synthetic timing self-test validates the on/off boundaries for every pattern without requiring a physical CAN bus, passive buzzer, timer PWM channel, or GPIO output.

## Temperature-Sensor Configuration

Both MCP9808 sensors share the same I²C bus on Board 1:

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

## SPI Flash Configuration

The W25Q64 connects to SPI2 on Board 1.

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

The W25Q64 driver models:

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

The firmware sends:

```text
0x9F
```

and reads three JEDEC identification bytes.

The connected device currently reports:

```text
0xEF4017
```

indicating successful communication with the connected W25Q64 flash device.

## Reserved Flash Test Sector

The final 4-KiB flash sector is reserved for development self-testing.

```text
Start address: 0x7FF000
End address:   0x7FFFFF
```

This region is not used by the persistent diagnostic logger.

The `FLASH TEST` command:

1. refreshes the watchdog,
2. erases the reserved sector,
3. refreshes the watchdog,
4. programs a known 32-byte pattern,
5. refreshes the watchdog,
6. reads the pattern back,
7. compares every byte,
8. reports success or failure.

The hardware flash self-test has been successfully verified using the connected W25Q64.

## Persistent Diagnostic Logging

`DiagnosticLogger` stores structured nonvolatile event records in the first two W25Q64 sectors.

```text
Start address: 0x000000
End address:   0x001FFF
Region size:   8192 bytes
```

Each diagnostic record is 32 bytes.

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

The two-sector region holds 256 records.

Current event types:

```text
1 = SystemStartup
2 = FaultActivated
3 = FaultCleared
```

The logger scans the complete region during startup and classifies each slot as erased, valid, or invalid. The scan reconstructs the number of valid records, latest valid sequence, next sequence number, next erased write location, and whether the region is full.

A startup record stores:

```text
data0 = primary ResetCause enum value
data1 = complete reset-cause mask
```

Fault-transition records store:

```text
data0 = fault bits that changed
data1 = complete active-fault mask afterward
```

The logger does not automatically erase old history when full.

`LOG ERASE` explicitly erases only the two diagnostic sectors. The final sector at `0x7FF000` remains reserved for `FLASH TEST`.

## Cooperative Scheduling

Board 1 task periods include:

```text
Button sampling:       5 ms
Heartbeat update:    500 ms
Health check:       1000 ms
Temperature sample: 1000 ms
Telemetry:          1000 ms
Watchdog refresh:    500 ms
CAN transmit:        500 ms
```

The application uses `HAL_GetTick()` and reusable `PeriodicTimer` objects rather than blocking delays for normal periodic scheduling.

## Automatic Heartbeat

The Board 1 on-board LD2 LED is an automatic system heartbeat.

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

The heartbeat cannot be manually disabled during normal operation.

This makes the heartbeat a consistent indication that normal application scheduling is continuing.

## USER Button

The Board 1 USER button is a local diagnostic input.

A debounced press:

```text
increments button_presses
        ↓
immediately transmits current UART telemetry
```

The button does not control heartbeat state.

Board 2's USER button is reserved for a future actuator self-test or warning-acknowledgement function.

## Independent Watchdog

Board 1 refreshes the independent watchdog every 500 milliseconds during normal execution.

The `WATCHDOG TEST` command deliberately stops watchdog refreshes so hardware reset behavior can be verified.

The watchdog is configured for an approximately 2-second timeout using the STM32 independent low-speed clock.

Flash self-test operations refresh the watchdog between destructive flash operations so flash testing does not interfere with watchdog recovery behavior.

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

## Fault Monitoring

Board 1 fault bits:

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

## UART Configuration

Both nodes use USART2 through their ST-LINK virtual COM ports.

```text
115200 baud
8 data bits
No parity
1 stop bit
No flow control
```

## Board 1 UART Telemetry

Current Board 1 telemetry fields include:

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

## Board 1 UART Line Handling

Either character completes an input command:

```text
Carriage return: \r
Line feed:       \n
```

CR, LF, and CR+LF terminals are supported.

For CR+LF, the second terminator produces an empty line, which is ignored.

## Board 1 UART Commands

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

### Flash Self-Test

```text
FLASH TEST
```

A successful test responds:

```text
OK FLASH TEST PASSED
```

### Diagnostic Log Erase

```text
LOG ERASE
```

Erases the two 4-KiB sectors reserved for persistent diagnostic records without affecting the final `FLASH TEST` sector.

### CAN Transmit Test

```text
CAN TEST
```

Requests an immediate Vehicle Health Status frame transmission.

A successful queue operation responds:

```text
OK CAN FRAME QUEUED
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

### Watchdog Test

```text
WATCHDOG TEST
```

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

commands remain removed because heartbeat operation is automatic.

## Development Tools

- STM32CubeMX
- STM32CubeIDE
- GNU Arm Embedded Toolchain
- Git
- PuTTY or another serial terminal
- Digital multimeter for bench diagnostics

## Current Verification Summary

Verified on Board 1 hardware:

- automatic heartbeat
- USER-button diagnostic snapshot
- dual MCP9808 I²C communication
- redundant/degraded temperature monitoring
- runtime fault detection and injection
- independent watchdog reset behavior
- reset-cause reporting
- W25Q64 JEDEC identification
- SPI erase/program/read self-test
- persistent diagnostic logging across reset
- CAN1 internal loopback
- UART command and telemetry handling

Verified in the current two-project software structure:

- Board 1 project builds
- Board 2 project builds
- shared CAN protocol header is consumed by both firmware targets
- Board 1 CAN1 initializes in normal mode
- Board 1 can queue the Vehicle Health Status frame
- Board 2 CAN1 initializes in normal mode
- Board 2 UART startup path runs
- Board 2 receive/decode framework compiles and runs
- Board 2 stores the latest valid remote vehicle-health state
- Board 2 remote-status software self-test validates decoding and timeout behavior without a physical CAN bus
- Board 2 communication supervision distinguishes waiting, connected, and communication-lost states
- Board 2 thermal-control self-test validates `NORMAL`, `WARM`, `COOLING`, `HIGH`, `CRITICAL`, and `SAFE`
- Board 2 thermal state defaults to `SAFE` while real CAN data is unavailable
- Board 2 actuator-command self-test validates cooling-duty, RGB-color, and buzzer-pattern mappings for every thermal state
- Board 2 safe-state actuator command requests 100% target cooling with distinct warning outputs
- Board 2 buzzer-pattern timing self-test validates `OFF`, `SLOW_BEEP`, `FAST_BEEP`, and double-beep `FAULT` timing

Pending physical validation:

- external CAN transceiver operation
- valid differential CANH/CANL signaling
- Board 2 receipt of Board 1 `0x100` frames
- CAN acknowledgment between nodes
- sustained periodic two-node communication
- fault and temperature propagation over the physical bus

## Future Two-Node Behavior

Board 1 will transmit system-health and temperature information over CAN.

Board 2 will use that information to determine the required thermal-control response.

Planned thermal states include:

```text
NORMAL
WARM
COOLING
HIGH
CRITICAL
SAFE
```

Board 2 now defines software target cooling duties of 0%, 40%, 70%, or 100% depending on thermal state. Physical PWM generation and fan-drive validation remain future hardware steps.

Board 2 also defines RGB warning-color and passive-buzzer pattern commands in software. The passive-buzzer pattern timing is now implemented as a non-blocking software sequencer; physical tone generation and buzzer driving remain future hardware steps.

Board 2 already contains software-level remote communication supervision. It tracks whether it is still waiting for its first valid frame, currently connected, or has exceeded the communication timeout after previously receiving valid traffic. Physical timeout behavior will be validated after the replacement CAN transceivers are installed.

Both nodes will ultimately supervise communication health.

Loss of communication will cause the affected node to report a communication fault and transition to defined safe behavior.

Board 2 will transmit its own health, actuator status, and fault information back to Board 1.

Board 1 will eventually store important remote-node events in persistent SPI flash.

## Design Principles

- Separate generated hardware code from application-owned C++
- Keep hardware configuration in each board's `.ioc` file
- Keep shared inter-node protocol definitions outside board-specific firmware
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
- Keep persistent diagnostic logging separate from the destructive flash-test sector
- Use fixed-size persistent records with integrity validation
- Log fault transitions rather than repeatedly storing steady-state faults
- Reconstruct persistent logger state from flash after reset
- Keep CAN message definitions explicit and fixed-width
- Keep the low-level CAN transport separate from application message policy
- Share CAN message layout definitions between nodes
- Store decoded remote-node state separately from low-level CAN transport
- Keep thermal-control policy separate from CAN transport and frame decoding
- Base actuator policy on trusted application state rather than raw network bytes
- Keep actuator command policy separate from physical GPIO/PWM implementation
- Represent cooling, visual warning, and audible warning as explicit commands
- Keep warning-pattern timing non-blocking so the main loop can continue servicing CAN and other tasks
- Separate buzzer envelope timing from future audio-frequency PWM generation
- Use conservative full-cooling behavior when required remote data is unavailable
- Treat software duty-cycle targets as unvalidated until the physical fan is characterized
- Use explicit communication states instead of treating missing data as valid data
- Use wraparound-safe elapsed-time comparisons for communication supervision
- Enter a defined safe state when required remote data is stale or invalid
- Validate software transport paths independently from physical CAN hardware
- Do not claim physical CAN communication as verified until both nodes exchange and acknowledge frames on the real bus
- Continue operating when a noncritical external peripheral is unavailable
- Preserve watchdog recovery behavior during peripheral operations
- Use defined safe-state behavior for future distributed-node failures
