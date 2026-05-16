#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <jansson.h>

#define PROTOCOL_VERSION 1
#define MESSAGE_ID_LEN 37

bool protocol_send_json(int socket, json_t *message);
bool protocol_send_server(int socket, int seq, const char *reply_to,
                          const char *type, json_t *payload);
json_t *protocol_server_message(int seq, const char *reply_to,
                                const char *type, json_t *payload);
json_t *protocol_client_message(const char *id, const char *session,
                                const char *type, json_t *payload);
json_t *protocol_parse(const char *line, char *error, size_t error_size);
const char *protocol_get_string(json_t *object, const char *key);
json_t *protocol_get_payload(json_t *message);
bool protocol_validate_version(json_t *message);
bool protocol_is_known_client_type(const char *type);
bool protocol_validate_client_message(json_t *message, bool require_session,
                                      char *error, size_t error_size);

#endif /* PROTOCOL_H */
