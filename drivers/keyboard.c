#include "drivers/keyboard.h"
#include "cpu/ports.h"
#include "cpu/isr.h"
#include "kernel/vga.h"

#define SC_MAX 57
const char sc_name[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 
        'U', 'I', 'O', 'P', '[', ']', '\n', '?', 'A', 'S', 'D', 'F', 'G', 
        'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z', 'X', 'C', 'V', 
        'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' '};
const char sc_ascii[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '?', '?', 'q', 'w', 'e', 'r', 't', 'y', 
        'u', 'i', 'o', 'p', '[', ']', '?', '?', 'a', 's', 'd', 'f', 'g', 
        'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'z', 'x', 'c', 'v', 
        'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' '};

#define KBD_BUF_SIZE 256
char kbd_buffer[KBD_BUF_SIZE];
volatile int kbd_head = 0;
volatile int kbd_tail = 0;

static void keyboard_callback(registers_t *regs) {
    uint8_t scancode = port_byte_in(0x60);
    if (scancode > SC_MAX) return;
    
    char letter = 0;
    if (scancode == 0x0E) letter = '\b';
    else if (scancode == 0x1C) letter = '\n';
    else letter = sc_ascii[(int)scancode];

    if (letter && letter != '?') {
        int next_head = (kbd_head + 1) % KBD_BUF_SIZE;
        if (next_head != kbd_tail) {
            kbd_buffer[kbd_head] = letter;
            kbd_head = next_head;
        }
    }
}

void init_keyboard(void) {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}

char keyboard_getchar(void) {
    if (kbd_head == kbd_tail) return 0;
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return c;
}
