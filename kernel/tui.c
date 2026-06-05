#include "kernel/tui.h"
#include "kernel/vga.h"
#include "kernel/util.h"
#include "kernel/auth.h"
#include "drivers/keyboard.h"
#include "fs/vfs.h"
#include "task/task.h"

#define BOX_HLINE 0xC4
#define BOX_VLINE 0xB3
#define BOX_UL 0xDA
#define BOX_UR 0xBF
#define BOX_LL 0xC0
#define BOX_LR 0xD9

void tui_clear_screen(uint8_t bg_color) {
    uint8_t color = bg_color << 4 | VGA_COLOR_WHITE;
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            vga_putentryat(' ', color, x, y);
        }
    }
    vga_setcolor(VGA_COLOR_LIGHT_GREY);
}

void tui_draw_text(int x, int y, const char* text, uint8_t bg_color, uint8_t fg_color) {
    uint8_t color = bg_color << 4 | fg_color;
    for (int i = 0; text[i] != '\0'; i++) {
        vga_putentryat(text[i], color, x + i, y);
    }
}

void tui_draw_window(int x, int y, int width, int height, const char* title, uint8_t bg_color, uint8_t fg_color) {
    uint8_t color = bg_color << 4 | fg_color;
    
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            vga_putentryat(' ', color, x + i, y + j);
        }
    }
    
    vga_putentryat(BOX_UL, color, x, y);
    vga_putentryat(BOX_UR, color, x + width - 1, y);
    vga_putentryat(BOX_LL, color, x, y + height - 1);
    vga_putentryat(BOX_LR, color, x + width - 1, y + height - 1);
    
    for (int i = 1; i < width - 1; i++) {
        vga_putentryat(BOX_HLINE, color, x + i, y);
        vga_putentryat(BOX_HLINE, color, x + i, y + height - 1);
    }
    
    for (int j = 1; j < height - 1; j++) {
        vga_putentryat(BOX_VLINE, color, x, y + j);
        vga_putentryat(BOX_VLINE, color, x + width - 1, y + j);
    }
    
    int title_len = string_length((char*)title);
    int title_x = x + (width - title_len) / 2;
    tui_draw_text(title_x, y, title, bg_color, fg_color);
}

static void read_input_tui(char *buffer, int max_len, int hide, int x, int y, uint8_t bg, uint8_t fg) {
    int idx = 0;
    buffer[0] = '\0';
    while (1) {
        char c = keyboard_getchar();
        if (c == '\n') {
            break;
        } else if (c == '\b') {
            if (idx > 0) {
                idx--;
                buffer[idx] = '\0';
                vga_putentryat(' ', bg << 4 | fg, x + idx, y);
            }
        } else if (c) {
            if (idx < max_len - 1) {
                buffer[idx] = c;
                if (hide) {
                    vga_putentryat('*', bg << 4 | fg, x + idx, y);
                } else {
                    vga_putentryat(c, bg << 4 | fg, x + idx, y);
                }
                idx++;
                buffer[idx] = '\0';
            }
        }
        switch_task();
    }
}

void tui_start_setup(void) {
    tui_clear_screen(VGA_COLOR_BLUE);
    tui_draw_window(20, 8, 40, 10, " First Time Setup ", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    tui_draw_text(22, 10, "Create Root Account", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE);
    tui_draw_text(22, 12, "Username: ", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    tui_draw_text(22, 14, "Password: ", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    char user[16];
    char pass[16];
    read_input_tui(user, 16, 0, 32, 12, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    read_input_tui(pass, 16, 1, 32, 14, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    auth_create_user(user, pass);
    tui_draw_text(22, 16, "Account created! Press Enter.", VGA_COLOR_LIGHT_GREY, VGA_COLOR_GREEN);
    while (keyboard_getchar() != '\n') { switch_task(); }
}

void tui_start_login(void) {
    while (1) {
        tui_clear_screen(VGA_COLOR_BLUE);
        tui_draw_window(25, 8, 30, 8, " Neuro-OS Login ", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        tui_draw_text(27, 10, "User: ", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        tui_draw_text(27, 12, "Pass: ", VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        
        char user[16];
        char pass[16];
        read_input_tui(user, 16, 0, 33, 10, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        read_input_tui(pass, 16, 1, 33, 12, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        
        if (auth_login(user, pass)) {
            break;
        } else {
            tui_draw_text(27, 14, "Invalid! Press Enter.", VGA_COLOR_LIGHT_GREY, VGA_COLOR_RED);
            while (keyboard_getchar() != '\n') { switch_task(); }
        }
    }
}

static int finder_selected = 0;
static int finder_dir = -1;

void tui_start_finder(void) {
    tui_clear_screen(VGA_COLOR_BLUE);
    char title_buf[64] = " Finder - ";
    int t_len = string_length(title_buf);
    const char* user = auth_get_current_username();
    int u_len = string_length((char*)user);
    memory_copy((uint8_t*)user, (uint8_t*)(title_buf + t_len), u_len);
    title_buf[t_len + u_len] = ' ';
    title_buf[t_len + u_len + 1] = '\0';
    tui_draw_window(10, 3, 60, 18, title_buf, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    while (1) {
        int total = get_vfs_file_count();
        uint32_t current_uid = auth_get_current_uid();
        
        // Collect visible items in current directory
        int visible_indices[MAX_FILES + 1];
        int num_visible = 0;
        
        if (finder_dir != -1) {
            visible_indices[num_visible++] = -2; // -2 means '..'
        }
        
        for (int i = 0; i < total; i++) {
            fs_node_t *node = vfs_get_node(i);
            if (node->parent_index == finder_dir && (node->owner_uid == 0 || node->owner_uid == current_uid)) {
                visible_indices[num_visible++] = i;
            }
        }
        
        if (finder_selected >= num_visible) finder_selected = (num_visible > 0) ? num_visible - 1 : 0;
        
        // Clear background
        for (int i = 0; i < 16; i++) {
            for(int k=0; k<58; k++) vga_putentryat(' ', VGA_COLOR_LIGHT_GREY<<4 | VGA_COLOR_BLACK, 11+k, 4+i);
        }
        
        // Draw files
        for (int i = 0; i < num_visible; i++) {
            uint8_t bg = (i == finder_selected) ? VGA_COLOR_CYAN : VGA_COLOR_LIGHT_GREY;
            uint8_t fg = (i == finder_selected) ? VGA_COLOR_WHITE : VGA_COLOR_BLACK;
            
            for(int k=0; k<58; k++) vga_putentryat(' ', bg<<4 | fg, 11+k, 4+i);
            
            if (visible_indices[i] == -2) {
                tui_draw_text(12, 4 + i, "[..] (Go Back)", bg, fg);
            } else {
                fs_node_t *node = vfs_get_node(visible_indices[i]);
                char name_buf[40];
                int l = string_length((char*)node->name);
                memory_copy((uint8_t*)node->name, (uint8_t*)name_buf, l);
                name_buf[l] = '\0';
                if (node->type == FS_DIR) {
                    name_buf[l] = '/'; name_buf[l+1] = '\0';
                }
                tui_draw_text(12, 4 + i, name_buf, bg, fg);
            }
        }
        
        char c = keyboard_getchar();
        if (c == 'w') {
            if (finder_selected > 0) finder_selected--;
        } else if (c == 's') {
            if (finder_selected < num_visible - 1) finder_selected++;
        } else if (c == '\n') {
            if (num_visible > 0) {
                int idx = visible_indices[finder_selected];
                if (idx == -2) {
                    fs_node_t *parent_node = vfs_get_node(finder_dir);
                    if (parent_node) finder_dir = parent_node->parent_index;
                    finder_selected = 0;
                } else {
                    fs_node_t *node = vfs_get_node(idx);
                    if (node->type == FS_DIR) {
                        finder_dir = idx;
                        finder_selected = 0;
                    } else {
                        tui_draw_window(15, 6, 50, 12, (char*)node->name, VGA_COLOR_CYAN, VGA_COLOR_WHITE);
                        int tx = 17, ty = 8;
                        for (uint32_t i = 0; i < node->size; i++) {
                            if (node->data[i] == '\n') { ty++; tx = 17; }
                            else {
                                vga_putentryat(node->data[i], VGA_COLOR_CYAN<<4 | VGA_COLOR_WHITE, tx++, ty);
                                if (tx > 62) { tx = 17; ty++; }
                            }
                        }
                        tui_draw_text(17, 16, "Press Enter to close.", VGA_COLOR_CYAN, VGA_COLOR_LIGHT_BROWN);
                        while (keyboard_getchar() != '\n') { switch_task(); }
                        tui_draw_window(10, 3, 60, 18, title_buf, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                    }
                }
            }
        } else if (c == 'q') {
            break;
        }
        switch_task();
    }
    vga_clear();
}
