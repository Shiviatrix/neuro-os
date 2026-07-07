#include "kernel/shell.h"
#include "kernel/vga.h"
#include "kernel/util.h"
#include "neuro/neuro.h"
#include "drivers/keyboard.h"
#include "drivers/timer.h"
#include "task/task.h"
#include "fs/vfs.h"
#include "mm/memory.h"
#include "kernel/auth.h"
#include "kernel/tui.h"

char key_buffer[256];
static int shell_current_dir = -1;

void vga_list_files_callback(const char *name) {
    vga_writestring(name);
}

void shell_process_char(char c) {
    if (c == '\b') {
        int len = string_length(key_buffer);
        if (len > 0) {
            backspace(key_buffer);
            vga_backspace();
        }
    } else if (c == '\n') {
        vga_writestring("\n");
        if (strcmp(key_buffer, "help") == 0) {
            vga_writestring("Available Commands:\n");
            vga_writestring("  help       - Display detailed descriptions of all available commands.\n");
            vga_writestring("  clear      - Clear the terminal screen of all output.\n");
            vga_writestring("  neuro      - Step the Liquid State Machine (LSM) Neural Network.\n");
            vga_writestring("  neuro_reset- Reset the LSM weights, topology, and state.\n");
            vga_writestring("  ls         - List files and subdirectories in the current directory.\n");
            vga_writestring("  cd [dir]   - Change current directory ('..' goes back, '/' goes to root).\n");
            vga_writestring("  cat [file] - Read and print the contents of a text file.\n");
            vga_writestring("  ps         - Display active running tasks in the cooperative scheduler.\n");
            vga_writestring("  free       - Show free heap memory (kmalloc memory pool status).\n");
            vga_writestring("  echo [txt] - Print the provided text back to the terminal.\n");
            vga_writestring("  mkdir [dir]- Create a new subdirectory in the current directory.\n");
            vga_writestring("  touch [file]- Create a new empty file.\n");
            vga_writestring("  write [f] [t]- Write text into a new file.\n");
            vga_writestring("  uname      - Display kernel and OS version information.\n");
            vga_writestring("  whoami     - Display your currently authenticated username.\n");
            vga_writestring("  uptime     - Show total elapsed time since the OS booted.\n");
            vga_writestring("  finder     - Launch Graphical Text-Mode File Manager to navigate files.\n");
        } else if (strcmp(key_buffer, "clear") == 0) {
            vga_clear();
        } else if (strcmp(key_buffer, "neuro") == 0) {
            neuro_step_and_print();
        } else if (strcmp(key_buffer, "neuro_reset") == 0) {
            neuro_init();
            vga_writestring("Neuro engine reset.\n");
        } else if (strcmp(key_buffer, "ls") == 0) {
            vfs_list_files_in_dir(vga_list_files_callback, shell_current_dir);
        } else if (strncmp(key_buffer, "cd ", 3) == 0) {
            char *dirname = key_buffer + 3;
            if (strcmp(dirname, "/") == 0) {
                shell_current_dir = -1;
            } else if (strcmp(dirname, "..") == 0) {
                if (shell_current_dir != -1) {
                    fs_node_t *parent_node = vfs_get_node(shell_current_dir);
                    if (parent_node) shell_current_dir = parent_node->parent_index;
                }
            } else {
                fs_node_t *dir = vfs_find_file_in_dir(dirname, shell_current_dir);
                if (dir && dir->type == FS_DIR) {
                    for (int i = 0; i < get_vfs_file_count(); i++) {
                        if (vfs_get_node(i) == dir) { shell_current_dir = i; break; }
                    }
                } else {
                    vga_writestring("cd: directory not found\n");
                }
            }
        } else if (strncmp(key_buffer, "cat ", 4) == 0) {
            char *filename = key_buffer + 4;
            fs_node_t *node = vfs_find_file_in_dir(filename, shell_current_dir);
            if (node && node->type == FS_FILE) {
                vga_writestring((char*)node->data);
            } else {
                vga_writestring("cat: file not found or is a directory\n");
            }
        } else if (strcmp(key_buffer, "cat") == 0) {
            vga_writestring("usage: cat [filename]\n");
        } else if (strcmp(key_buffer, "ps") == 0) {
            vga_writestring("PID   STATE   NAME\n");
            vga_writestring("0     RUN     kernel_idle\n");
            vga_writestring("1     RUN     shell_task\n");
            vga_writestring("2     RUN     neuro_task\n");
        } else if (strcmp(key_buffer, "free") == 0) {
            vga_writestring("Heap memory: ");
            char buf[16];
            int_to_ascii(get_free_memory(), buf);
            vga_writestring(buf);
            vga_writestring(" bytes free / ");
            int_to_ascii(get_total_memory(), buf);
            vga_writestring(buf);
            vga_writestring(" bytes total\n");
        } else if (strncmp(key_buffer, "echo ", 5) == 0) {
            vga_writestring(key_buffer + 5);
            vga_writestring("\n");
        } else if (strcmp(key_buffer, "echo") == 0) {
            vga_writestring("\n");
        } else if (strncmp(key_buffer, "mkdir ", 6) == 0) {
            char *dirname = key_buffer + 6;
            if (vfs_find_file_in_dir(dirname, shell_current_dir) != NULL) {
                vga_writestring("mkdir: name already exists\n");
            } else {
                int ret = vfs_add_dir(dirname, auth_get_current_uid(), shell_current_dir);
                if (ret == -1) vga_writestring("mkdir: out of space\n");
            }
        } else if (strncmp(key_buffer, "touch ", 6) == 0) {
            char *filename = key_buffer + 6;
            if (vfs_find_file_in_dir(filename, shell_current_dir) != NULL) {
                vga_writestring("touch: name already exists\n");
            } else {
                vfs_add_file_full(filename, "", auth_get_current_uid(), shell_current_dir);
            }
        } else if (strncmp(key_buffer, "write ", 6) == 0) {
            char *args = key_buffer + 6;
            int space_idx = -1;
            for(int i=0; args[i] != '\0'; i++) {
                if(args[i] == ' ') { space_idx = i; break; }
            }
            if(space_idx != -1) {
                args[space_idx] = '\0';
                char *filename = args;
                char *content = args + space_idx + 1;
                if (vfs_find_file_in_dir(filename, shell_current_dir) != NULL) {
                    vga_writestring("write: file already exists\n");
                } else {
                    vfs_add_file_full(filename, content, auth_get_current_uid(), shell_current_dir);
                }
            } else {
                vga_writestring("usage: write [filename] [content]\n");
            }
        } else if (strcmp(key_buffer, "uname") == 0) {
            vga_writestring("Neuro-OS v3.0 (i686-elf)\n");
        } else if (strcmp(key_buffer, "whoami") == 0) {
            vga_writestring(auth_get_current_username());
            vga_writestring("\n");
        } else if (strcmp(key_buffer, "finder") == 0) {
            tui_start_finder();
            vga_writestring("Exited finder.\n");
        } else if (strcmp(key_buffer, "uptime") == 0) {
            vga_writestring("Uptime: ");
            char buf[16];
            int_to_ascii(timer_get_tick() / 50, buf);
            vga_writestring(buf);
            vga_writestring(" seconds\n");
        } else if (string_length(key_buffer) > 0) {
            vga_writestring("Unknown command: ");
            vga_writestring(key_buffer);
            vga_writestring("\n");
        }
        key_buffer[0] = '\0';
        vga_writestring("> ");
    } else {
        int len = string_length(key_buffer);
        if (len < 255) {
            append(key_buffer, c);
            vga_putchar(c);
        }
    }
}

void shell_task(void) {
    if (!auth_has_users()) {
        tui_start_setup();
    }
    tui_start_login();
    vga_clear();

    vga_writestring("Neuro-OS shell v4.0 (TUI)\n");
    vga_writestring("Logged in as: ");
    vga_writestring(auth_get_current_username());
    vga_writestring("\n> ");
    key_buffer[0] = '\0';
    
    while(1) {
        char c = keyboard_getchar();
        if (c) {
            shell_process_char(c);
        }
        switch_task();
    }
}
