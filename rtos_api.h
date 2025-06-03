#ifndef RTOS_API_H
#define RTOS_API_H

#include <stdint.h>

typedef struct {
    // opaque handle
} rtos_task_t;

void rtos_task_delay_ms(uint32_t ms);
void rtos_task_create(rtos_task_t* task, void (*task_fn)(void*), void* arg);
void rtos_mutex_lock(void);
void rtos_mutex_unlock(void);

#endif // RTOS_API_H