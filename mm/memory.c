#include "mm/memory.h"
#include "kernel/util.h"
#include "cpu/isr.h"
#include "kernel/vga.h"

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

static void page_fault_handler(registers_t *r) {
    uint32_t faulting_address;
    __asm__ __volatile__("mov %%cr2, %0" : "=r" (faulting_address));
    vga_writestring("Page fault at 0x");
    char buf[12];
    int_to_ascii(faulting_address, buf);
    vga_writestring(buf);
    vga_writestring("\n");
    while(1);
}

void init_paging(void) {
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }
    for(int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 3;
    }
    page_directory[0] = ((uint32_t)first_page_table) | 3;

    register_interrupt_handler(14, page_fault_handler);

    __asm__ __volatile__("mov %0, %%cr3" :: "r"(page_directory));
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ __volatile__("mov %0, %%cr0" :: "r"(cr0));
}

typedef struct block_meta {
    size_t size;
    struct block_meta *next;
    uint8_t free;
} block_meta_t;

static block_meta_t *heap_start = NULL;
static uint32_t total_heap = 0;

void init_heap(uint32_t start, uint32_t size) {
    heap_start = (block_meta_t *)start;
    heap_start->size = size - sizeof(block_meta_t);
    heap_start->next = NULL;
    heap_start->free = 1;
    total_heap = size;
}

void *kmalloc(size_t size) {
    size = (size + 3) & ~3; // 4-byte align
    block_meta_t *current = heap_start;
    while (current) {
        if (current->free && current->size >= size) {
            if (current->size > size + sizeof(block_meta_t) + 16) {
                block_meta_t *new_block = (block_meta_t *)((uint8_t*)(current + 1) + size);
                new_block->free = 1;
                new_block->size = current->size - size - sizeof(block_meta_t);
                new_block->next = current->next;
                current->next = new_block;
                current->size = size;
            }
            current->free = 0;
            return (void *)(current + 1);
        }
        current = current->next;
    }
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_meta_t *block = (block_meta_t *)ptr - 1;
    block->free = 1;
}

uint32_t get_free_memory(void) {
    uint32_t free_mem = 0;
    block_meta_t *current = heap_start;
    while (current) {
        if (current->free) free_mem += current->size;
        current = current->next;
    }
    return free_mem;
}

uint32_t get_total_memory(void) {
    return total_heap;
}
