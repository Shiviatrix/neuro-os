#include "task/task.h"
#include "mm/memory.h"
#include "kernel/util.h"

#define MAX_TASKS 4
pcb_t tasks[MAX_TASKS];
int current_task = -1;
int num_tasks = 0;

extern void task_switch_asm(uint32_t *old_esp, uint32_t new_esp);

void init_tasking(void) {
    memory_set((uint8_t*)tasks, 0, sizeof(tasks));
    tasks[0].id = 0;
    tasks[0].active = 1;
    current_task = 0;
    num_tasks = 1;
}

void create_task(void (*entry)(void)) {
    if (num_tasks >= MAX_TASKS) return;
    
    uint32_t stack = (uint32_t)kmalloc(4096);
    uint32_t *stack_top = (uint32_t *)(stack + 4096);
    
    *(--stack_top) = (uint32_t)entry; 
    *(--stack_top) = 0; 
    *(--stack_top) = 0; 
    *(--stack_top) = 0; 
    *(--stack_top) = 0; 
    
    int id = num_tasks++;
    tasks[id].id = id;
    tasks[id].esp = (uint32_t)stack_top;
    tasks[id].ebp = 0;
    tasks[id].eip = (uint32_t)entry;
    tasks[id].active = 1;
}

void switch_task(void) {
    if (num_tasks <= 1) return;
    
    int old_task = current_task;
    current_task = (current_task + 1) % num_tasks;
    
    task_switch_asm(&tasks[old_task].esp, tasks[current_task].esp);
}
