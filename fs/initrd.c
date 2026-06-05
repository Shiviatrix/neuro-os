#include "fs/initrd.h"
#include "fs/vfs.h"

void init_initrd(void) {
    vfs_init();
    vfs_add_file("readme.txt", "Welcome to Neuro-OS!\nThis is a public file.\n", 0);
    vfs_add_file("secret.log", "Nothing to see here... just a test file.\n", 0);
    vfs_add_file("neuro.conf", "learning_rate=0.01\nhidden_layers=2\n", 0);
    vfs_add_file("root_diary.txt", "Dear Diary, today I built an OS.", 1);
    
    int docs_dir = vfs_add_dir("docs", 0, -1);
    vfs_add_file_full("hebbian.txt", "Hebbian learning: neurons that fire together, wire together.\nIt strengthens connections dynamically.", 0, docs_dir);
    int private_dir = vfs_add_dir("private", 1, -1);
    vfs_add_file_full("ssh.key", "ssh-rsa AAAAB3N...", 1, private_dir);
}
