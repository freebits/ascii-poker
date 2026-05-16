#ifndef IDS_H
#define IDS_H

#include <stddef.h>
#include <stdbool.h>

#define UUID_STRING_LEN 37

bool generate_uuid_string(char *buffer, size_t size);
bool generate_session_token(char *buffer, size_t size, size_t random_bytes);

#endif /* IDS_H */
