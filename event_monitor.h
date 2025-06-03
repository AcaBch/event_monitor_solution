#ifndef EVENT_MONITOR_H
#define EVENT_MONITOR_H

#include <stdint.h>

// Initialize the event monitor with a bitmask of pins to monitor
void event_monitor_init(uint32_t monitored_mask);

// User-implemented function to handle event count reports
void report_event_count(uint32_t count);

#endif // EVENT_MONITOR_H