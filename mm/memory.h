#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void init_paging(void);
void init_heap(uint32_t start, uint32_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);
uint32_t get_free_memory(void);
uint32_t get_total_memory(void);

#endif
