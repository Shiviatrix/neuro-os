#ifndef TASK_H
#define TASK_H

#include <stdint.h>

typedef struct {
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t id;
    uint8_t active;
} pcb_t;

void init_tasking(void);
void create_task(void (*entry)(void));
void switch_task(void);

extern int num_tasks;

#endif
