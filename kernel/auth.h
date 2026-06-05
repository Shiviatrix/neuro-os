#ifndef AUTH_H
#define AUTH_H
#include <stdint.h>

#define MAX_USERS 4
#define MAX_USERNAME 16

typedef struct {
    uint32_t uid;
    char username[MAX_USERNAME];
    uint8_t password_hash[32];
    uint8_t active;
} user_t;

void auth_init(void);
int auth_create_user(const char* username, const char* password);
int auth_login(const char* username, const char* password);
uint32_t auth_get_current_uid(void);
const char* auth_get_current_username(void);
int auth_has_users(void);

#endif
