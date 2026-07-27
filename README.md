# Embedded Vehicle Health Controller

A bare-metal embedded C++ firmware project for the
STMicroelectronics NUCLEO-F446RE development board.

The project simulates a vehicle subsystem controller that collects sensor
data, tracks system state, detects and reports faults, receives commands,
and transmits telemetry.

## Project Status

Board bring-up, mixed C/C++ integration, GPIO output control, debounced
button input, cooperative scheduling, TIM7 hardware interrupts, health
monitoring, USART2 telemetry output, and interrupt-driven UART reception are
complete.

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
- Interrupt-driven one-byte USART2 reception
- A fixed-size single-producer/single-consumer receive ring buffer
- Main-loop assembly of newline-terminated input
- UART receive, overflow, and dropped-data diagnostics
- No runtime dynamic allocation in application code

The application currently runs four cooperative periodic activities:

```text
Button sampling task    every 5 ms
Heartbeat task          every 500 ms
Health-monitoring task  every 1000 ms
Telemetry task          every 1000 ms
```

USART2 receive processing also runs on every application-loop pass so
interrupt-buffered bytes are drained promptly.

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
- Communication interface: USART2
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
10. One-byte USART2 interrupt reception is started.
11. The main loop repeatedly calls `Application::run()`.
12. The button task samples and debounces the USER button.
13. The heartbeat task toggles LD2 while enabled.
14. The health task evaluates button-task and TIM7 activity.
15. TIM7 interrupts increment an interrupt-side counter.
16. The main loop observes hardware-timer events.
17. The telemetry task transmits a structured status line once per second.
18. USART2 receive-complete interrupts transfer received bytes into a fixed
    ring buffer.
19. The receive callback immediately rearms the next one-byte reception.
20. The main loop removes buffered bytes and assembles input lines.
21. Carriage-return characters are ignored.
22. Newline characters complete the current input line.
23. Complete non-empty lines are counted and retained for later command
    parsing.
24. Oversized lines and dropped input are recorded in diagnostics.

## USART2 Configuration

USART2 is configured as:

```text
Mode:              Asynchronous
Baud rate:         115200
Word length:       8 bits
Parity:            None
Stop bits:         1
Hardware flow:     None
Transmit pin:      PA2
Receive pin:       PA3
Global interrupt:  Enabled
```

The ST-LINK virtual COM port connects USART2 to a PC through the board’s
existing USB connection.

## Telemetry Output

The telemetry task runs every:

```text
1000 milliseconds
```

Example output:

```text
uptime_ms=5000 button_presses=2 heartbeat_enabled=1 healthy=1 timer_active=1 timer_irq_count=50 rx_lines=2 rx_dropped_bytes=0 rx_overflow_lines=0 rx_errors=0
```

Fields include:

```text
uptime_ms
    Milliseconds since HAL startup.

button_presses
    Number of confirmed USER-button presses.

heartbeat_enabled
    1 when heartbeat blinking is enabled.

healthy
    1 when monitored application conditions are healthy.

timer_active
    1 when TIM7 produced interrupts during the latest health interval.

timer_irq_count
    Total observed TIM7 interrupt count.

rx_lines
    Number of complete input lines consumed by the application.

rx_dropped_bytes
    Bytes discarded because the interrupt receive ring was full.

rx_overflow_lines
    Lines discarded because they exceeded the supported length.

rx_errors
    UART errors or failures to restart interrupt reception.
```

## UART Receive Architecture

UART receive behavior is implemented in:

```text
App/Inc/UartCommandReceiver.hpp
App/Src/UartCommandReceiver.cpp
```

The receive path is:

```text
PC serial terminal
    |
    v
USART2 receives one byte
    |
    v
USART2_IRQHandler()
    |
    v
HAL_UART_IRQHandler()
    |
    v
HAL_UART_RxCpltCallback()
    |
    v
application_uart_byte_received()
    |
    v
Application::onUartByteReceived()
    |
    v
UartCommandReceiver ring buffer
    |
    v
Application::processUartReceive()
    |
    v
Complete newline-terminated line
```

The interrupt callback performs only bounded work:

```text
Forward received byte
Rearm one-byte reception
Record a restart error when necessary
Return
```

Line assembly and validation occur in main-loop context.

## Receive Ring Buffer

The receiver uses a fixed-size byte ring:

```text
Capacity: 128 bytes
Producer: USART2 interrupt
Consumer: cooperative main loop
```

The interrupt writes at the head index.

The main loop reads at the tail index.

When advancing the head would collide with the tail, the new byte is
discarded and the dropped-byte count increases.

No heap allocation is used.

## Input Line Handling

The maximum stored line capacity is:

```text
64 bytes
```

This supports:

```text
63 command characters
1 null terminator
```

Input handling rules are:

```text
'\r'
    Ignored.

'\n'
    Completes the current line.

Empty line
    Ignored.

Line longer than 63 characters
    Entire line discarded and overflow counter incremented.

Complete line while another is pending
    New line discarded and dropped-line counter incremented.
```

Commands are received and counted in the current revision, but they are not
yet interpreted.

## UART Telemetry Architecture

Telemetry behavior is implemented in:

```text
App/Inc/UartTelemetry.hpp
App/Src/UartTelemetry.cpp
```

The transmit path is:

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

The current transmit path uses a bounded polling call. The receive path is
interrupt-driven.

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

App/Inc/UartCommandReceiver.hpp
App/Src/UartCommandReceiver.cpp
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
│   │   ├── UartCommandReceiver.hpp
│   │   ├── UartTelemetry.hpp
│   │   └── application_bridge.h
│   └── Src/
│       ├── Application.cpp
│       ├── ButtonDebouncer.cpp
│       ├── DigitalOutput.cpp
│       ├── PeriodicTimer.cpp
│       ├── UartCommandReceiver.cpp
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

## Cooperative Execution

The firmware currently uses one main execution context.

`Application::run()` performs work in this order:

```text
1. Observe TIM7 events
2. Process interrupt-buffered UART input
3. Run the button task when due
4. Run the heartbeat task when due
5. Run the health task when due
6. Run the telemetry task when due
```

UART input processing is not periodic. It runs on every pass so the receive
ring is drained promptly.

## Interrupt Responsibilities

### TIM7 interrupt

```text
Increment timer interrupt counter
Return
```

### USART2 receive-complete callback

```text
Forward one byte into the receive ring
Restart one-byte reception
Return
```

### USART2 error callback

```text
Record receive error
Attempt to restart reception
Return
```

Command interpretation, telemetry formatting, health checks, button
processing, and line assembly remain outside interrupt context.

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

## Generated-Code Source of Truth

The `.ioc` file is the source of truth for hardware configuration.

Required hardware settings include:

```text
PA5          GPIO_Output
PA2          USART2_TX
PA3          USART2_RX
TIM7         Activated
USART2       Asynchronous
TIM7 IRQ     Enabled
USART2 IRQ   Enabled
```

Manual generated-code hardware changes may be overwritten during future code
generation.

## Build and Run

1. Open the `.ioc` file in STM32CubeMX.
2. Confirm PA5 remains configured as `GPIO_Output`.
3. Confirm USART2 remains configured as asynchronous.
4. Confirm PA2 and PA3 are assigned to USART2.
5. Confirm USART2 uses 115200 8-N-1.
6. Enable the USART2 global interrupt.
7. Confirm TIM7 configuration remains intact.
8. Generate code.
9. Build the project in STM32CubeIDE.
10. Connect the board through the ST-LINK USB connector.
11. Open the ST-LINK virtual COM port at 115200 8-N-1.
12. Run the firmware.
13. Confirm telemetry appears once per second.
14. Type a line and press Enter.
15. Confirm `rx_lines` increases.
16. Confirm normal input leaves receive diagnostics at zero.
17. Confirm existing button, heartbeat, health, and TIM7 behavior remains
    functional.

## Debugging

Useful receive values include:

```text
uartReceiver_.receiveHead_
uartReceiver_.receiveTail_
uartReceiver_.workingLine_
uartReceiver_.workingLineLength_
uartReceiver_.workingLineOverflow_
uartReceiver_.pendingLine_
uartReceiver_.pendingLineAvailable_
receivedLine_
receivedLineCount_
```

Useful diagnostics include:

```text
uartReceiver_.droppedByteCount_
uartReceiver_.overflowLineCount_
uartReceiver_.droppedLineCount_
uartReceiver_.receiveErrorCount_
```

Breakpoints pause the processor and can cause UART overruns. Remove receive
breakpoints before judging normal behavior.

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
- Keep interrupt handlers and callbacks short.
- Transfer asynchronous data into fixed-size buffers.
- Defer parsing and substantial processing to the main loop.
- Use fixed-size buffers for communication.
- Track dropped, malformed, and failed communication.
- Avoid indefinite waits.
- Avoid runtime dynamic allocation.
- Keep cooperative tasks short and bounded.
- Monitor task and hardware-event liveness.
- Use rollover-safe time calculations.
- Separate raw hardware inputs from confirmed application state.
- Represent one-time transitions as events.
- Prefer composition for application components.
- Add abstractions when they improve ownership, clarity, or testability.

## Planned Features

The firmware will be expanded to include:

- Command parsing and validation
- Supported UART controller commands
- Command response messages
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
- Input model: Periodic GPIO polling and interrupt-driven UART reception
- UART receive model: One-byte interrupt reception with ring buffering
- UART transmit model: Bounded polling transmission
- Telemetry interface: USART2 through ST-LINK virtual COM
- Timing model: Non-blocking periodic scheduling with bounded UART transmit
- Interrupt model: Minimal ISR notification with main-loop processing
- Hardware timer: TIM7 periodic update interrupt
- Health model: Task and hardware-event liveness monitoring
- RTOS: Not currently used
- Dynamic allocation: Avoided during normal firmware operation