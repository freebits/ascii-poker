#include "ids.h"

#include <openssl/rand.h>
#include <stdio.h>
#include <uuid/uuid.h>

bool generate_uuid_string(char *buffer, size_t size) {
    if (!buffer || size < UUID_STRING_LEN) {
        return false;
    }

    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, buffer);
    return true;
}

bool generate_session_token(char *buffer, size_t size, size_t random_bytes) {
    if (!buffer || random_bytes == 0 || size < random_bytes * 2 + 1) {
        return false;
    }

    unsigned char bytes[64];
    if (random_bytes > sizeof(bytes)) {
        return false;
    }

    if (RAND_bytes(bytes, (int)random_bytes) != 1) {
        return false;
    }

    for (size_t i = 0; i < random_bytes; i++) {
        snprintf(buffer + i * 2, 3, "%02x", bytes[i]);
    }
    buffer[random_bytes * 2] = '\0';
    return true;
}
