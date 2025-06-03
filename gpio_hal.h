#ifndef GPIO_HAL_H
#define GPIO_HAL_H

#include <stdint.h>

typedef uint32_t gpio_mask_t;

// Reads the current GPIO input state (32-bit)
gpio_mask_t gpio_read_input(void);

// Registers a callback to be called on GPIO change
void gpio_register_callback(void (*callback)(gpio_mask_t new_state));

#endif // GPIO_HAL_H