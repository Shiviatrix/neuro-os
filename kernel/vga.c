#include "kernel/vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static size_t vga_row;
static size_t vga_column;
static uint8_t vga_color_attr;
static uint16_t* vga_buffer;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

void vga_init(void) {
	vga_row = 0;
	vga_column = 0;
	vga_color_attr = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	vga_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			vga_buffer[index] = vga_entry(' ', vga_color_attr);
		}
	}
}

void vga_setcolor(uint8_t color) {
	vga_color_attr = color;
}

void vga_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	vga_buffer[index] = vga_entry(c, color);
}

static void vga_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y - 1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', vga_color_attr);
    }
    vga_row = VGA_HEIGHT - 1;
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_column = 0;
        if (++vga_row == VGA_HEIGHT) {
            vga_scroll();
        }
        return;
    }
	vga_putentryat(c, vga_color_attr, vga_column, vga_row);
	if (++vga_column == VGA_WIDTH) {
		vga_column = 0;
		if (++vga_row == VGA_HEIGHT) {
            vga_scroll();
        }
	}
}

void vga_backspace(void) {
    if (vga_column == 0 && vga_row > 0) {
        vga_row--;
        vga_column = VGA_WIDTH - 1;
    } else if (vga_column > 0) {
        vga_column--;
    }
    vga_putentryat(' ', vga_color_attr, vga_column, vga_row);
}

void vga_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		vga_putchar(data[i]);
}

void vga_writestring(const char* data) {
    size_t len = 0;
    while (data[len]) len++;
	vga_write(data, len);
}

void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', vga_color_attr);
        }
    }
    vga_row = 0;
    vga_column = 0;
}
