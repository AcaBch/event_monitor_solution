#include <assert.h>
#include <stdio.h>
#include "event_monitor.h"
#include "gpio_hal.h"
#include "rtos_api.h"

// Mock state and test tracking
static gpio_mask_t simulated_state = 0;
static void (*static_callback)(gpio_mask_t new_state) = NULL;
static uint32_t total_events_counted = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Mocks for HAL functions
gpio_mask_t gpio_read_input(void) {
    return simulated_state;
}

void gpio_register_callback(void (*callback)(gpio_mask_t new_state)) {
    static_callback = callback;
}

// Mocks for RTOS functions
void rtos_mutex_lock(void) {}
void rtos_mutex_unlock(void) {}
void rtos_task_delay_ms(uint32_t ms) { (void)ms; }
void rtos_task_create(rtos_task_t* task, void (*task_fn)(void*), void* arg) { 
    (void)task; (void)task_fn; (void)arg; 
}

// Test helper to access internal event count (for testing only)
extern volatile uint32_t event_count;  // Access the static variable from event_monitor.c

// Helper function to manually trigger event reporting for testing
void trigger_event_report_for_test(void) {
    // Simulate what the monitor_task would do
    rtos_mutex_lock();
    uint32_t count = event_count;
    event_count = 0;
    rtos_mutex_unlock();
    
    // Report the count
    report_event_count(count);
}

// User-implemented function for testing
void report_event_count(uint32_t count) {
    total_events_counted += count;
    printf("  -> Events reported: %u (Total so far: %u)\n", count, total_events_counted);
}

// Helper function to simulate GPIO changes
void simulate_gpio_change(gpio_mask_t new_state) {
    simulated_state = new_state;
    if (static_callback) {
        static_callback(new_state);
    }
}

// Helper function to reset test state
void reset_test_state(void) {
    simulated_state = 0;
    total_events_counted = 0;
    static_callback = NULL;
    // Reset the internal event counter
    rtos_mutex_lock();
    event_count = 0;
    rtos_mutex_unlock();
}

// Helper function to check test results
void check_test_result(const char* test_name, uint32_t expected, uint32_t actual) {
    printf("  %s: Expected %u, Actual %u -> ", test_name, expected, actual);
    if (expected == actual) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
        tests_failed++;
    }
}

// Test functions with proper verification
void test_rising_edge_detection() {
    printf("\n1. Testing rising edge detection...\n");
    
    reset_test_state();
    
    // Initialize monitor with mask 0x0F (monitor first 4 bits)
    event_monitor_init(0x0F);
    
    // Simulate GPIO changes and track expected events
    simulate_gpio_change(0x00); // Initial state: all low
    simulate_gpio_change(0x03); // Rising edges on bits 0 and 1 (2 events)
    simulate_gpio_change(0x07); // Rising edge on bit 2 (1 event)
    simulate_gpio_change(0x07); // No change, no new events
    simulate_gpio_change(0x05); // Falling edge on bit 1, no new rising edges
    simulate_gpio_change(0x0F); // Rising edges on bits 1 and 3 (2 events)
    
    // Manually trigger event reporting for testing
    trigger_event_report_for_test();
    
    // Total expected: 2 + 1 + 0 + 0 + 2 = 5 events
    check_test_result("Rising edge detection", 5, total_events_counted);
}

void test_mask_filtering() {
    printf("\n2. Testing mask filtering...\n");
    
    reset_test_state();
    
    // Initialize monitor with mask 0x05 (monitor only bits 0 and 2)
    event_monitor_init(0x05);
    
    // Simulate GPIO changes
    simulate_gpio_change(0x00); // Initial state
    simulate_gpio_change(0x0F); // Rising edges on all bits, but only 0 and 2 should count
    
    // Manually trigger event reporting for testing
    trigger_event_report_for_test();
    
    // Expected: 2 events (bits 0 and 2 only)
    check_test_result("Mask filtering", 2, total_events_counted);
}

void test_no_monitored_pins() {
    printf("\n3. Testing with no monitored pins...\n");
    
    reset_test_state();
    
    // Initialize monitor with mask 0x00 (monitor no pins)
    event_monitor_init(0x00);
    
    // Simulate GPIO changes
    simulate_gpio_change(0x00);
    simulate_gpio_change(0xFF); // Rising edges on all bits, but none monitored
    
    // Manually trigger event reporting for testing
    trigger_event_report_for_test();
    
    // Expected: 0 events
    check_test_result("No monitored pins", 0, total_events_counted);
}

void test_edge_transition_logic() {
    printf("\n4. Testing edge transition logic...\n");
    
    reset_test_state();
    
    // Initialize monitor with all bits
    event_monitor_init(0xFF);
    
    // Test sequence: demonstrate rising edge detection
    simulate_gpio_change(0x00); // All low
    simulate_gpio_change(0x01); // Bit 0: 0->1 (rising edge) - 1 event
    simulate_gpio_change(0x03); // Bit 1: 0->1 (rising edge) - 1 event
    simulate_gpio_change(0x01); // Bit 1: 1->0 (falling edge, ignored) - 0 events
    simulate_gpio_change(0x03); // Bit 1: 0->1 (rising edge again) - 1 event
    
    // Manually trigger event reporting for testing
    trigger_event_report_for_test();
    
    // Expected: 1 + 1 + 0 + 1 = 3 events
    check_test_result("Edge transition logic", 3, total_events_counted);
}

void test_multiple_bits_simultaneous() {
    printf("\n5. Testing multiple bits changing simultaneously...\n");
    
    reset_test_state();
    
    // Initialize monitor with mask 0xFF (all bits)
    event_monitor_init(0xFF);
    
    // Test simultaneous rising edges
    simulate_gpio_change(0x00); // All low
    simulate_gpio_change(0xFF); // All bits rise simultaneously
    
    // Manually trigger event reporting for testing
    trigger_event_report_for_test();
    
    // Expected: 8 events (all 8 bits from 0 to 1)
    check_test_result("Multiple simultaneous edges", 8, total_events_counted);
}

void test_partial_mask_with_mixed_transitions() {
    printf("\n6. Testing partial mask with mixed transitions...\n");
    
    reset_test_state();
    
    // Initialize monitor with mask 0xF0 (monitor upper 4 bits only)
    event_monitor_init(0xF0);
    
    simulate_gpio_change(0x00); // All low
    simulate_gpio_change(0x0F); // Lower 4 bits rise (not monitored) - 0 events
    simulate_gpio_change(0xFF); // Upper 4 bits rise (monitored) - 4 events
    simulate_gpio_change(0x0F); // Upper 4 bits fall (ignored) - 0 events
    simulate_gpio_change(0xFF); // Upper 4 bits rise again - 4 events
    
    // Manually trigger event reporting for testing
    trigger_event_report_for_test();
    
    // Expected: 0 + 4 + 0 + 4 = 8 events
    check_test_result("Partial mask mixed transitions", 8, total_events_counted);
}

void print_test_summary() {
    int total_tests = tests_passed + tests_failed;
    
    printf("\n----------------------------------------\n");
    printf("TEST SUMMARY\n");
    printf("----------------------------------------\n");
    printf("Total tests run: %d\n", total_tests);
    printf("Tests passed:    %d\n", tests_passed);
    printf("Tests failed:    %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\nALL TESTS PASSED!\n");
        printf("The GPIO event monitor implementation is working correctly.\n");
    } else {
        printf("\nSOME TESTS FAILED!\n");
        printf("Please review the implementation for issues.\n");
    }
}

int main() {
    printf("\nStarting Event Monitor Unit Tests\n");
    printf("========================================\n");
    printf("Testing embedded firmware GPIO rising edge detection\n");
    printf("Formula: rising_edges = (~previous_state & new_state) & monitored_mask\n");
    
    test_rising_edge_detection();
    test_mask_filtering();
    test_no_monitored_pins();
    test_edge_transition_logic();
    test_multiple_bits_simultaneous();
    test_partial_mask_with_mixed_transitions();
    
    print_test_summary();
    
    return (tests_failed == 0) ? 0 : 1; // Return 0 for success, 1 for failure
}