#include "task/task.h"
#include "mm/memory.h"
#include "kernel/util.h"
#include "neuro/neuro.h"

#define MAX_TASKS 4
pcb_t tasks[MAX_TASKS];
int current_task = -1;
int num_tasks = 0;

static uint32_t task_rand_state = 0x98765432;
static uint32_t task_rand(void) {
    task_rand_state ^= task_rand_state << 13;
    task_rand_state ^= task_rand_state >> 17;
    task_rand_state ^= task_rand_state << 5;
    return task_rand_state;
}
static float task_frand01(void) {
    return (float)(task_rand() & 0xFFFFFF) / (float)0xFFFFFF;
}

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
    
    // Neural Lottery Scheduler
    float scores[MAX_TASKS];
    float total_v = 0.0f;
    
    for (int i = 0; i < num_tasks; i++) {
        if (!tasks[i].active) {
            scores[i] = 0.0f;
            continue;
        }
        // Base priority
        scores[i] = 0.1f;
        
        // Neuro task (id 2) and idle task (id 0) need baseline priority 
        // to prevent OS freeze if they aren't stimulated enough.
        if (i == 0) {
            scores[i] = 0.2f;
        } else if (i == 2) {
            scores[i] = 1.0f; // High priority for the brain!
        } else {
            // Map task to Output Neurons (56 + i)
            float v = neuro_get_voltage(56 + i);
            scores[i] += v; 
        }
        total_v += scores[i];
    }
    
    float r = task_frand01() * total_v;
    int next_task = 0;
    
    for (int i = 0; i < num_tasks; i++) {
        if (!tasks[i].active) continue;
        r -= scores[i];
        if (r <= 0.0f) {
            next_task = i;
            break;
        }
    }
    
    current_task = next_task;
    
    if (current_task != old_task) {
        task_switch_asm(&tasks[old_task].esp, tasks[current_task].esp);
    }
}
