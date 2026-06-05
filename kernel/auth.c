#include "kernel/auth.h"
#include "kernel/util.h"
#include "crypto/sha256.h"
#include "kernel/vga.h"

static user_t users[MAX_USERS];
static uint32_t current_uid = 0;
static int user_count = 0;

void auth_init(void) {
    for (int i = 0; i < MAX_USERS; i++) {
        users[i].active = 0;
    }
    user_count = 0;
    current_uid = 0;
}

int auth_has_users(void) {
    return user_count > 0;
}

static void hash_password(const char* password, uint8_t* hash_out) {
    sha256_ctx ctx;
    sha256_init(&ctx);
    const uint8_t salt[] = {0x12, 0x34, 0x56, 0x78};
    sha256_update(&ctx, salt, sizeof(salt));
    sha256_update(&ctx, (const uint8_t*)password, string_length((char*)password));
    sha256_final(&ctx, hash_out);
}

int auth_create_user(const char* username, const char* password) {
    if (user_count >= MAX_USERS) return -1;
    
    int idx = user_count;
    users[idx].uid = idx + 1; // UIDs start at 1
    memory_set((uint8_t*)users[idx].username, 0, MAX_USERNAME);
    int len = string_length((char*)username);
    if (len >= MAX_USERNAME) len = MAX_USERNAME - 1;
    memory_copy((uint8_t*)username, (uint8_t*)users[idx].username, len);
    
    hash_password(password, users[idx].password_hash);
    users[idx].active = 1;
    user_count++;
    return 0;
}

int auth_login(const char* username, const char* password) {
    uint8_t input_hash[32];
    hash_password(password, input_hash);
    
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].active && strcmp(users[i].username, (char*)username) == 0) {
            int match = 1;
            for (int j = 0; j < 32; j++) {
                if (users[i].password_hash[j] != input_hash[j]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                current_uid = users[i].uid;
                return 1;
            }
        }
    }
    return 0;
}

uint32_t auth_get_current_uid(void) {
    return current_uid;
}

const char* auth_get_current_username(void) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].active && users[i].uid == current_uid) {
            return users[i].username;
        }
    }
    return "guest";
}
