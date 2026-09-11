# Embedded Vehicle Health Controller

A bare-metal embedded C++ prototype for the STM32 NUCLEO-F446RE that models a distributed vehicle-health and thermal-control system across two STM32 nodes.

Board 1 monitors vehicle-oriented system health, acquires redundant temperature measurements, detects and records runtime faults, persists diagnostic events in external SPI flash, supports diagnostic fault injection, recovers from application stalls through an independent watchdog, reports reset causes, processes serial commands, and transmits periodic vehicle-health status frames over CAN.

Board 2 is a remote thermal/actuator-control node. It receives and decodes Board 1's Vehicle Health Status frames, supervises CAN communication freshness, selects a thermal-control state, drives a PWM cooling fan, RGB warning LED, and passive-buzzer warning output, and enters a conservative safe state when required remote data becomes stale or unavailable.

The project uses STM32CubeMX-generated hardware initialization together with separate application-owned C++ layers. Generated C code communicates with each board's C++ application through a small C-compatible bridge.

The current hardware is a bench prototype. Two colocated MCP9808 temperature sensors on Board 1 simulate redundant vehicle battery-temperature channels. Board 1 internal CAN loopback and physical two-node CAN communication have both been verified. The two STM32 nodes exchange the shared `0x100` Vehicle Health Status frame over a real 500-kbit/s CAN bus through SN65HVD230 transceivers.

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
- Automatic recovery from CAN startup/HAL error state with bounded 500-ms retry attempts
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
- Physical TIM3 PWM control of a common-cathode RGB warning LED
- Physical TIM3 PWM control of a MOSFET-switched 5 V cooling fan
- TIM4 channel 1 audio-frequency PWM support for a passive buzzer
- Non-blocking buzzer envelope timing driving the physical buzzer PWM enable state
- Debounced Board 2 USER button actuator self-test
- Startup grace period that suppresses brief reset-time buzzer and SAFE-output artifacts
- Non-blocking actuator self-test that preserves CAN servicing and communication supervision
- Safe-state output of magenta warning plus 100% cooling and the fault buzzer pattern when trusted remote status is unavailable

## Current CAN Validation Status

Board 1 CAN1 internal loopback has been verified successfully. That test demonstrated the STM32 bxCAN controller, receive filter, transmit mailbox path, receive FIFO path, standard identifier handling, payload serialization, and byte-for-byte receive verification without requiring an external transceiver.

Physical two-node CAN communication is also verified. Both NUCLEO-F446RE boards run CAN1 in normal mode at 500 kbit/s through SN65HVD230 transceivers. Board 1 periodically transmits the shared standard-ID `0x100` Vehicle Health Status frame every 500 ms, and Board 2 receives and decodes the live system-health, temperature-validity, sensor-availability, selected-temperature, and fault-mask fields.

Verified behavior includes Board 2 transitioning from `SAFE` to `NORMAL` after valid room-temperature data is received, driving the RGB warning LED green, and reducing the cooling command from the safe-state 100% duty to 0%. Loss of valid Board 1 traffic for more than 1500 ms transitions Board 2 to `COMMUNICATION_LOST` and back to the defined `SAFE` actuator policy.

Board 2 also implements automatic CAN recovery. During physical bring-up, a startup-order condition could leave the HAL CAN handle in an error state after a start timeout. The `CanBus` service now detects when CAN is not in the listening state and performs a bounded recovery sequence with 500-ms retry spacing. This allows Board 2 to recover without requiring a manual reset when the remote node becomes available.

## Planned Features

- Persistent CAN communication and remote-node event history
- Bidirectional CAN heartbeat supervision
- Board 2 watchdog supervision
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
- 4-pin common-cathode RGB warning LED with three 330-ohm current-limiting resistors
- 5 V two-wire brushless cooling fan
- N-channel logic-level MOSFET for low-side fan switching
- 100-ohm MOSFET gate resistor and 10-kilohm gate pull-down resistor
- 5 V passive buzzer
- N-channel logic-level MOSFET for low-side buzzer switching
- 100-ohm buzzer MOSFET gate resistor and 10-kilohm gate pull-down resistor
- 1N4007 flyback diode across the buzzer
- Physical PWM-controlled cooling fan

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
        ├── RGB warning LED PWM output
        ├── cooling-fan PWM output through MOSFET
        ├── passive-buzzer tone PWM output through MOSFET
        ├── USER-button actuator self-test
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

The verified normal runtime behavior is periodic transmission every:

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

A synthetic self-test validates every thermal state without requiring CAN transceiver hardware. During normal runtime, the selected state is also mapped into real fan and RGB LED outputs through the actuator-command policy.

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

`SAFE` requests full cooling because stale or unavailable temperature data should not silently disable cooling. The target percentages are applied to the physical fan through TIM3 channel 4.

The `40%` and `70%` cooling values are current control-policy targets. The physical fan has been verified to respond to 40%, 70%, and 100% commands on the bench.

A synthetic actuator-command self-test verifies every mapping without requiring the physical CAN transceivers, fan, MOSFET, RGB LED, or buzzer.

## Board 2 Passive-Buzzer Output

Board 2 converts the high-level buzzer command into two separate timing layers:

```text
ActuatorCommandPolicy
        ↓
BuzzerPattern
        ↓
BuzzerPatternSequencer
        ↓
outputActive()
        ↓
BuzzerPwm
        ↓
TIM4_CH1
        ↓
MOSFET
        ↓
5 V passive buzzer
```

The software envelope controls when the buzzer should be audible:

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

The physical tone uses a separate timer because TIM3 is already shared by the RGB LED and cooling fan. Board 2 uses:

```text
PB6 = TIM4_CH1 = buzzer PWM command
```

TIM4 is configured for an approximately 2-kHz PWM tone using the 84-MHz APB1 timer clock:

```text
Prescaler = 83
Counter period = 499

84,000,000 / (83 + 1) = 1,000,000 timer counts/second
1,000,000 / (499 + 1) = 2,000 PWM periods/second
```

When the buzzer output is enabled, `BuzzerPwm` sets the channel compare value to half of the timer period, producing approximately 50% duty cycle. With `ARR = 499`, the compare value is:

```text
CCR1 = (499 + 1) / 2
     = 250
```

When disabled, the compare register is set to zero so the PWM output remains low.

The passive buzzer is driven through a low-side N-channel MOSFET rather than directly from the STM32 GPIO:

```text
NUCLEO +5V -> buzzer positive
buzzer negative -> MOSFET drain
MOSFET source -> GND

PB6 / TIM4_CH1 -> 100-ohm resistor -> MOSFET gate
MOSFET gate -> 10-kilohm resistor -> GND
NUCLEO GND -> shared circuit GND
```

A 1N4007 flyback diode is connected directly across the buzzer:

```text
diode cathode / striped end -> +5 V / buzzer positive
diode anode                 -> buzzer negative / MOSFET drain
```

The diode is normally reverse-biased and provides a current path for the buzzer's inductive energy when the MOSFET switches off.

The existing software timing self-test continues to validate the envelope boundaries independently of the physical PWM output.

## Board 2 RGB Warning LED

Board 2 now drives a physical 4-pin common-cathode RGB LED using three TIM3 PWM channels.

```text
PA6 = TIM3_CH1 = red channel
PA7 = TIM3_CH2 = green channel
PB0 = TIM3_CH3 = blue channel
```

Each color channel uses its own 330-ohm current-limiting resistor. The LED common cathode connects to GND.

TIM3 is configured for a 1-kHz PWM frequency using the 84-MHz APB1 timer clock:

```text
Prescaler = 839
Counter period = 99

84,000,000 / (839 + 1) = 100,000 timer counts/second
100,000 / (99 + 1) = 1,000 PWM periods/second
```

Because one PWM period contains 100 timer counts, the logical percentage values map naturally to PWM duty-cycle targets. The driver still calculates compare values from the timer auto-reload value rather than hard-coding that assumption.

Current RGB mapping:

```text
Warning color   Red   Green   Blue
GREEN            0%    100%     0%
YELLOW         100%     25%     0%
BLUE             0%      0%   100%
ORANGE         100%      5%     0%
RED            100%      0%     0%
MAGENTA        100%      0%    80%
```

`MAGENTA` is the runtime safe-state indication, distinguishing unavailable/stale remote data from the `RED` critical-temperature state. The mixed-color percentages are calibrated for the specific physical RGB LED used in this project.

The final calibrated mixed-color targets for this LED are Yellow `{100, 25, 0}`, Orange `{100, 5, 0}`, and Magenta `{100, 0, 80}` because the physical LED's channels do not have equal perceived brightness.

## Physical Two-Node CAN Integration

The project uses two Waveshare SN65HVD230 CAN transceiver boards to connect the STM32F446RE bxCAN controllers to a real two-node CAN bus.

Each node is wired as:

```text
NUCLEO 3.3V  -> transceiver 3.3V
NUCLEO GND   -> transceiver GND
PA12 CAN_TX  -> transceiver CAN TX
PA11 CAN_RX  <- transceiver CAN RX
```

Between the two transceiver boards:

```text
CANH -> CANH
CANL -> CANL
GND  -> GND
```

The Waveshare SN65HVD230 CAN Board includes a fixed 120-ohm termination resistor between CANH and CANL. With exactly two boards at the two ends of this project bus, both onboard termination resistors remain installed, producing approximately 60 ohms across CANH and CANL when the system is powered off. No additional external termination resistor is added.

Both bxCAN controllers use normal mode at 500 kbit/s:

```text
Prescaler = 6
BS1       = 11 TQ
BS2       = 2 TQ
SJW       = 1 TQ
```

With the 42 MHz CAN peripheral clock, this produces 500 kbit/s with a sample point of approximately 85.7%.

Board 1 periodically transmits standard identifier `0x100` every 500 ms. The eight-byte payload contains protocol version, health/status flags, selected temperature in 0.1 degree Celsius units, and the active fault mask.

Board 2 validates and decodes that frame, transitions its communication state from `WAITING_FOR_DATA` to `CONNECTED`, updates the thermal-control state machine, and applies the resulting cooling-fan and RGB warning commands. If valid frames stop arriving for more than 1500 ms, Board 2 transitions to `COMMUNICATION_LOST` and the thermal controller enters `SAFE`.

The temporary internal-loopback RX pull-up used during Board 1 controller-only bring-up is not used with the physical transceiver. PA11 is configured with no internal pull resistor in normal CAN operation.

## Board 2 Cooling-Fan PWM Output

Board 2 now drives the physical 5 V two-wire cooling fan through an N-channel MOSFET. The STM32 does not supply the fan current directly; it supplies only the MOSFET gate-control signal.

```text
PB1 = TIM3_CH4 = fan PWM command

NUCLEO +5V -> fan positive
fan negative -> MOSFET drain
MOSFET source -> GND

PB1 -> 100-ohm resistor -> MOSFET gate
MOSFET gate -> 10-kilohm resistor -> GND
NUCLEO GND -> shared circuit GND
```

TIM3 channel 4 shares the same 1-kHz timer base already used by the RGB LED channels. The four TIM3 channels therefore share the same counter, prescaler, and auto-reload value, while each channel has its own capture/compare register and duty cycle.

```text
TIM3_CH1 / CCR1 -> RGB red
TIM3_CH2 / CCR2 -> RGB green
TIM3_CH3 / CCR3 -> RGB blue
TIM3_CH4 / CCR4 -> cooling fan MOSFET gate
```

The fan driver accepts cooling commands as percentages and converts them to TIM3 compare values using the timer auto-reload value. The current actuator policy requests:

```text
NORMAL      0%
WARM        0%
COOLING    40%
HIGH       70%
CRITICAL  100%
SAFE      100%
```

`SAFE` deliberately requests full cooling because missing or stale remote temperature information should not silently disable cooling.

The fan driver keeps timer-register details inside the hardware abstraction while the application continues to express cooling demand as a percentage. The previously used blocking startup hardware self-test is no longer called during normal startup so CAN initialization and recovery are not delayed.

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

Board 2's USER button starts a non-blocking actuator self-test. The test is debounced in software and intentionally continues servicing CAN while the physical outputs are exercised.

The self-test sequence uses five three-second stages:

```text
Stage 0   fan 0%     GREEN     buzzer OFF
Stage 1   fan 40%    BLUE      buzzer OFF
Stage 2   fan 70%    ORANGE    SLOW_BEEP
Stage 3   fan 100%   RED       FAST_BEEP
Stage 4   fan 100%   MAGENTA   FAULT
```

After the final stage, Board 2 restores the actuator command associated with the current live thermal-control state. CAN reception and communication supervision continue throughout the test; the self-test temporarily overrides only the physical actuator outputs.

## Board 2 USER Button Actuator Self-Test

Board 2 uses the on-board USER button as a local diagnostic input. The button is sampled from PC13 and debounced for 30 ms before a press is accepted.

A valid press starts a five-stage actuator-output sequence without calling `HAL_Delay()`. Each stage lasts 3000 ms and is advanced from the normal main loop using `HAL_GetTick()`.

```text
PC13 USER button
      ↓
software debounce
      ↓
startActuatorSelfTest()
      ↓
non-blocking stage timer
      ↓
activeOutputCommand_
      ├── RGB LED PWM
      ├── cooling-fan PWM
      └── buzzer pattern / tone PWM
```

The live thermal-control state and CAN status continue to update in the background during the test. Changes in the remote state update the normal actuator policy, but do not overwrite the physical test outputs while the self-test is active.

When the final stage completes, the self-test clears its override and immediately restores the current actuator-policy command. This means the board returns to the correct `NORMAL`, `WARM`, `COOLING`, `HIGH`, `CRITICAL`, or `SAFE` output rather than returning to a hard-coded default.

Repeated presses while the self-test is already active are ignored.

At reset, Board 2 uses a short startup grace period before applying communication-failure outputs. During this grace period, the fan remains off and the buzzer remains silent while the node waits for the first valid CAN status frame. If communication is established during the grace period, the board transitions directly to the live thermal-control output. If no valid frame arrives before the grace period expires, the normal `SAFE` behavior is applied.

The USER button input is treated as active-low on PC13. The button starts unarmed after reset and must first be observed released for a short interval before a subsequent press can start the actuator self-test. This prevents startup transients or rapid resets from being interpreted as diagnostic button presses.

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
- Board 2 TIM3 RGB PWM output initializes on three channels
- Board 2 calibrated RGB warning colors have been verified on hardware
- Board 2 runtime `SAFE` state drives the RGB LED to `MAGENTA` without requiring physical CAN
- Board 2 TIM3 channel 4 fan PWM output initializes independently of the RGB channels
- Board 2 physical fan response has been verified at 40%, 70%, and 100% duty commands
- Board 2 runtime `SAFE` state requests 100% cooling duty without requiring physical CAN
- Board 2 TIM4 channel 1 buzzer PWM driver builds into the actuator-output path
- Board 2 buzzer envelope state now directly enables or disables the physical tone PWM
- Board 2 USER-button self-test software path cycles fan, RGB, and buzzer commands without blocking CAN servicing

Pending physical validation:

- passive-buzzer audible tone generation through TIM4_CH1 and the MOSFET stage
- `FAULT` double-beep behavior while Board 2 is in `SAFE`
- buzzer silence after valid room-temperature CAN traffic transitions Board 2 to `NORMAL`
- `SLOW_BEEP` and `FAST_BEEP` audible behavior under `HIGH` and `CRITICAL` thermal states

## Current Two-Node Behavior

Board 1 transmits system-health and temperature information over CAN.

Board 2 uses that information to determine the required thermal-control response:

```text
NORMAL
WARM
COOLING
HIGH
CRITICAL
SAFE
```

Board 2 commands cooling duties of 0%, 40%, 70%, or 100% depending on thermal state. The physical 5 V two-wire fan is driven through TIM3 PWM and an N-channel MOSFET power stage.

Board 2 also drives the physical common-cathode RGB warning LED through TIM3 PWM. The calibrated warning-color mapping is:

```text
NORMAL    -> GREEN
WARM      -> YELLOW
COOLING   -> BLUE
HIGH      -> ORANGE
CRITICAL  -> RED
SAFE      -> MAGENTA
```

The passive-buzzer warning-pattern policy, non-blocking envelope sequencer, and TIM4 audio-frequency PWM driver are implemented. Physical audible validation is the current hardware test step.

Board 2 supervises the freshness of the shared `0x100` status frame. It tracks whether it is waiting for its first valid frame, connected to Board 1, or has exceeded the 1500-ms communication timeout after previously receiving valid traffic.

Loss of valid remote communication causes Board 2 to enter `SAFE`, command 100% cooling, select the magenta warning indication, and use the fault buzzer pattern.

Board 2 also services CAN recovery continuously. If the HAL CAN peripheral is not in the listening state, the transport performs a bounded reinitialization attempt every 500 ms until communication can resume.

Future distributed-system work includes Board 2 transmitting its own health and actuator status back to Board 1, Board 1 supervising the remote node, and persistent logging of important remote-node events.

## Verified Physical CAN Behavior

The physical two-node CAN link has been validated with the following observed behavior:

```text
- Approximately 60 ohms across CANH and CANL with both boards powered off
- Both SN65HVD230 transceiver boards powered from 3.3 V
- CANH connected to CANH and CANL connected to CANL
- Shared ground between the two non-isolated nodes
- Board 1 PA12 -> CAN TX and PA11 <- CAN RX
- Board 2 PA12 -> CAN TX and PA11 <- CAN RX
- Both CAN controllers operating in normal mode at 500 kbit/s
- Board 1 periodic Vehicle Health Status transmission succeeds
- Board 2 receives and decodes real Board 1 temperature and fault data
- Board 2 `can_rx_count` increases with live traffic
- Board 2 transitions to `CONNECTED` and `NORMAL` for valid room-temperature data
- RGB output changes to green and fan duty changes to 0% in `NORMAL`
- Communication loss produces `COMMUNICATION_LOST` and `SAFE`
- `SAFE` commands magenta warning and 100% cooling
- Board 2 automatically recovers from the observed CAN startup/HAL error condition without requiring a manual reset
```

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
- Keep actuator self-tests non-blocking so communication supervision continues during local diagnostics
- Restore live actuator policy after a manual output override completes
- Keep reset/startup outputs deterministic and quiet until communication state is established
- Require the active-low USER button to be released before arming it after reset
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
- Separate buzzer envelope timing from audio-frequency PWM generation
- Use one independent current-limiting resistor per RGB LED channel
- Keep logical warning colors separate from timer/PWM details
- Convert normalized intensity percentages into timer compare values inside the RGB driver
- Keep motor-load current off the STM32 GPIO by using a MOSFET as the fan power switch
- Keep passive-buzzer load current off the STM32 GPIO by using a MOSFET as the buzzer power switch
- Use a dedicated timer for the passive-buzzer audio frequency so RGB and fan PWM timing remain independent
- Keep the flyback diode across the inductive buzzer load rather than across the complete 5 V supply
- Keep cooling commands expressed as percentages and isolate timer-register details inside the fan PWM driver
- Treat minimum reliable two-wire fan duty as a hardware-calibration value rather than assuming every commanded duty will start the fan
- Keep the CAN controller/protocol logic separate from the physical transceiver layer
- Recover CAN transport state with bounded retry timing rather than requiring a manual node reset
- Terminate a two-node CAN bus at both physical ends and avoid unnecessary extra termination
- Use a shared ground reference between the two non-isolated CAN nodes
- Fail Board 2 to the SAFE actuator policy when valid remote CAN status becomes stale
- Use conservative full-cooling behavior when required remote data is unavailable
- Treat software duty-cycle targets as unvalidated until the physical fan is characterized
- Use explicit communication states instead of treating missing data as valid data
- Use wraparound-safe elapsed-time comparisons for communication supervision
- Enter a defined safe state when required remote data is stale or invalid
- Validate software transport paths independently from physical CAN hardware
- Validate physical CAN communication with real two-node traffic, acknowledgment, timeout behavior, and actuator-state response
- Continue operating when a noncritical external peripheral is unavailable
- Preserve watchdog recovery behavior during peripheral operations
- Use defined safe-state behavior for distributed-node communication failures
