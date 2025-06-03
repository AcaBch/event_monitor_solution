# Event-Driven GPIO Monitor

## Overview

This firmware monitors GPIO inputs for rising edges, counts events, and reports them every 1000 ms via a task under a custom RTOS. It supports a 32-bit bitmask to filter which GPIO pins are monitored.

## Edge Detection Logic

Rising edges are detected using the formula:

```
rising_edges = (~previous_state & new_state) & monitored_mask;
```

This identifies transitions from 0 to 1 *only* for monitored pins.

## Build Instructions

Use a C99-compatible compiler. No external libraries are used.

### For Windows (using MinGW/gcc):
```cmd
gcc -Wall -Wextra -std=c99 -o test_event_monitor.exe test_event_monitor.c event_monitor.c user_implemented.c
.\test_event_monitor.exe
```

### For Linux/Mac:
```bash
gcc -Wall -Wextra -std=c99 -o test_event_monitor test_event_monitor.c event_monitor.c user_implemented.c
./test_event_monitor
```

## Files

- `event_monitor.c/h` – Core implementation
- `gpio_hal.h` – GPIO HAL interface (provided by hardware team)
- `rtos_api.h` – RTOS API interface (provided by RTOS team)
- `user_implemented.c` – User implementation of report_event_count()
- `test_event_monitor.c` – Unit tests with mocked HAL and RTOS functions
- `README.md` – This file

## Design Notes

### Thread Safety
- Mutex protects `event_count` between interrupt handler and RTOS task context
- Atomic read-and-reset operation ensures no events are lost

### Interrupt Handling
- GPIO callback registered once during initialization
- Rising edge detection uses efficient bit manipulation
- Only monitored pins (per bitmask) trigger event counting

### RTOS Integration
- Background task runs every 1000ms to report accumulated events
- No FreeRTOS or CMSIS used; only the provided custom API
- Task creation handled automatically during initialization

### Bit Manipulation Logic
The core rising edge detection logic:
```c
rising_edges = (~previous_state & new_state) & monitored_mask;
```
- `~previous_state & new_state`: Detects bits that transitioned from 0→1
- `& monitored_mask`: Filters to only monitored pins
- Result: 1 only where monitored pins had rising edges

## Testing

The unit tests verify:
- Rising edge detection accuracy
- Mask filtering functionality
- Proper handling of falling edges (ignored)
- Edge case scenarios (no monitored pins, multiple transitions)

Tests use mocked HAL and RTOS functions to run on any development system without requiring actual hardware.