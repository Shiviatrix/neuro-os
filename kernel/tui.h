#ifndef TUI_H
#define TUI_H
#include <stdint.h>

void tui_clear_screen(uint8_t bg_color);
void tui_draw_window(int x, int y, int width, int height, const char* title, uint8_t bg_color, uint8_t fg_color);
void tui_draw_text(int x, int y, const char* text, uint8_t bg_color, uint8_t fg_color);
void tui_start_setup(void);
void tui_start_login(void);
void tui_start_finder(void);

#endif
