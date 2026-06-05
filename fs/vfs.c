#include "fs/vfs.h"
#include "kernel/util.h"
#include "mm/memory.h"
#include "kernel/auth.h"

static fs_node_t files[MAX_FILES];
static int file_count = 0;

void vfs_init(void) {
    file_count = 0;
}

int vfs_add_dir(const char *name, uint32_t owner, int parent_index) {
    if (file_count >= MAX_FILES) return -1;
    int idx = file_count;
    int len = string_length((char*)name);
    if (len >= 32) len = 31;
    memory_copy((uint8_t*)name, (uint8_t*)files[idx].name, len);
    files[idx].name[len] = '\0';
    files[idx].size = 0;
    files[idx].owner_uid = owner;
    files[idx].data = NULL;
    files[idx].type = FS_DIR;
    files[idx].parent_index = parent_index;
    file_count++;
    return idx;
}

void vfs_add_file_full(const char *name, const char *content, uint32_t owner, int parent_index) {
    if (file_count >= MAX_FILES) return;
    int idx = file_count;
    int len = string_length((char*)name);
    if (len >= 32) len = 31;
    memory_copy((uint8_t*)name, (uint8_t*)files[idx].name, len);
    files[idx].name[len] = '\0';
    len = string_length((char*)content);
    files[idx].size = len;
    files[idx].owner_uid = owner;
    files[idx].type = FS_FILE;
    files[idx].parent_index = parent_index;
    files[idx].data = (uint8_t *)kmalloc(len + 1);
    if (files[idx].data) {
        memory_copy((uint8_t *)content, files[idx].data, len + 1);
        file_count++;
    }
}

void vfs_add_file(const char *name, const char *content, uint32_t owner) {
    vfs_add_file_full(name, content, owner, -1);
}

fs_node_t* vfs_find_file(const char *name) {
    return vfs_find_file_in_dir(name, -1);
}

fs_node_t* vfs_find_file_in_dir(const char *name, int parent_index) {
    uint32_t current_uid = auth_get_current_uid();
    for (int i = 0; i < file_count; i++) {
        if (files[i].parent_index == parent_index && strcmp(files[i].name, (char*)name) == 0) {
            if (files[i].owner_uid == 0 || files[i].owner_uid == current_uid) {
                return &files[i];
            }
        }
    }
    return NULL;
}

void vfs_list_files(void (*print_fn)(const char *)) {
    vfs_list_files_in_dir(print_fn, -1);
}

void vfs_list_files_in_dir(void (*print_fn)(const char *), int parent_index) {
    uint32_t current_uid = auth_get_current_uid();
    for (int i = 0; i < file_count; i++) {
        if (files[i].parent_index == parent_index) {
            if (files[i].owner_uid == 0 || files[i].owner_uid == current_uid) {
                print_fn(files[i].name);
                if (files[i].type == FS_DIR) print_fn("/");
                print_fn("\n");
            }
        }
    }
}

int get_vfs_file_count(void) {
    return file_count;
}

const char* vfs_get_filename(int index) {
    if (index >= 0 && index < file_count) {
        return files[index].name;
    }
    return "";
}

fs_node_t* vfs_get_node(int index) {
    if (index >= 0 && index < file_count) {
        return &files[index];
    }
    return NULL;
}
