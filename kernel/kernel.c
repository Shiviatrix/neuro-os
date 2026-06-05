#include "kernel/vga.h"
#include "cpu/gdt.h"
#include "cpu/isr.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "kernel/shell.h"
#include "neuro/neuro.h"
#include "mm/memory.h"
#include "task/task.h"
#include "fs/initrd.h"
#include "kernel/auth.h"
#include "kernel/tui.h"

void neuro_background_task(void) {
    while(1) {
        neuro_step(); 
        switch_task();
    }
}

void kernel_main(void) {
    vga_init();
    vga_writestring("Booting Neuro-OS...\n");

    init_gdt();
    isr_install();
    
    vga_writestring("Initializing Paging...\n");
    init_paging();

    vga_writestring("Initializing Heap...\n");
    init_heap(0x200000, 0x100000); // 1MB heap starting at 2MB
    
    vga_writestring("Initializing Auth System...\n");
    auth_init();

    vga_writestring("Initializing VFS and Initrd...\n");
    init_initrd();

    __asm__ __volatile__("sti");
    init_timer(50);
    init_keyboard();
    neuro_init();

    vga_writestring("Initializing Tasking...\n");
    init_tasking();
    create_task(shell_task);
    create_task(neuro_background_task);

    vga_writestring("Dropping into Multitasking Environment...\n");

    while (1) {
        switch_task();
    }
}
