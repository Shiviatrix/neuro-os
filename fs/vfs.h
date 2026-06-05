#ifndef VFS_H
#define VFS_H
#include <stdint.h>

#define MAX_FILES 16

typedef enum {
    FS_FILE = 0,
    FS_DIR = 1
} fs_type_t;

typedef struct {
    char name[32];
    uint32_t size;
    uint8_t *data;
    uint32_t owner_uid; // User isolation
    fs_type_t type;
    int parent_index; // -1 for root
} fs_node_t;

void vfs_init(void);
void vfs_add_file(const char *name, const char *content, uint32_t owner);
int vfs_add_dir(const char *name, uint32_t owner, int parent_index);
void vfs_add_file_full(const char *name, const char *content, uint32_t owner, int parent_index);
fs_node_t* vfs_find_file(const char *name);
fs_node_t* vfs_find_file_in_dir(const char *name, int parent_index);
void vfs_list_files(void (*print_fn)(const char *));
void vfs_list_files_in_dir(void (*print_fn)(const char *), int parent_index);
int get_vfs_file_count(void);
const char* vfs_get_filename(int index);
fs_node_t* vfs_get_node(int index);

#endif
