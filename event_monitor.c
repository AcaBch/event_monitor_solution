#include "event_monitor.h"
#include "gpio_hal.h"
#include "rtos_api.h"

volatile uint32_t event_count = 0;
static uint32_t monitored_mask = 0;
static gpio_mask_t previous_state = 0;

void gpio_change_callback(gpio_mask_t new_state) {
    gpio_mask_t rising_edges;
    int i;

    // Detect rising edges: bits that were 0 and are now 1, filtered by monitored mask
    rising_edges = (~previous_state & new_state) & monitored_mask;
    previous_state = new_state;

    if (rising_edges) {
        rtos_mutex_lock();
        // Count the number of rising edges
        for (i = 0; i < 32; ++i) {
            if (rising_edges & (1u << i)) {
                ++event_count;
            }
        }
        rtos_mutex_unlock();
    }
}

static void monitor_task(void* arg) {
    uint32_t count;
    
    (void)arg; // Suppress unused parameter warning
    
    while (1) {
        // Wait for 1000ms
        rtos_task_delay_ms(1000);

        // Atomically read and reset the counter
        rtos_mutex_lock();
        count = event_count;
        event_count = 0;
        rtos_mutex_unlock();

        // Report the count
        report_event_count(count);
    }
}

void event_monitor_init(uint32_t mask) {
    static rtos_task_t task;
    
    monitored_mask = mask;
    previous_state = gpio_read_input();
    gpio_register_callback(gpio_change_callback);

    // Create the monitoring task
    rtos_task_create(&task, monitor_task, NULL);
}