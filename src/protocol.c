#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

static bool send_all(int socket, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(socket, data + sent, len - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += (size_t)n;
    }
    return true;
}

bool protocol_send_json(int socket, json_t *message) {
    if (!message) {
        return false;
    }

    char *dump = json_dumps(message, JSON_COMPACT);
    if (!dump) {
        return false;
    }

    bool ok = send_all(socket, dump, strlen(dump)) &&
              send_all(socket, "\n", 1);
    free(dump);
    return ok;
}

json_t *protocol_server_message(int seq, const char *reply_to,
                                const char *type, json_t *payload) {
    if (!type) {
        return NULL;
    }
    if (!payload) {
        payload = json_object();
    }
    if (!payload) {
        return NULL;
    }

    json_t *message = json_object();
    if (!message) {
        json_decref(payload);
        return NULL;
    }

    json_object_set_new(message, "v", json_integer(PROTOCOL_VERSION));
    json_object_set_new(message, "seq", json_integer(seq));
    if (reply_to && reply_to[0]) {
        json_object_set_new(message, "reply_to", json_string(reply_to));
    }
    json_object_set_new(message, "type", json_string(type));
    json_object_set_new(message, "payload", payload);
    return message;
}

bool protocol_send_server(int socket, int seq, const char *reply_to,
                          const char *type, json_t *payload) {
    json_t *message = protocol_server_message(seq, reply_to, type, payload);
    if (!message) {
        return false;
    }

    bool ok = protocol_send_json(socket, message);
    json_decref(message);
    return ok;
}

json_t *protocol_client_message(const char *id, const char *session,
                                const char *type, json_t *payload) {
    if (!id || !type) {
        return NULL;
    }
    if (!payload) {
        payload = json_object();
    }
    if (!payload) {
        return NULL;
    }

    json_t *message = json_object();
    if (!message) {
        json_decref(payload);
        return NULL;
    }

    json_object_set_new(message, "v", json_integer(PROTOCOL_VERSION));
    json_object_set_new(message, "id", json_string(id));
    if (session && session[0]) {
        json_object_set_new(message, "session", json_string(session));
    }
    json_object_set_new(message, "type", json_string(type));
    json_object_set_new(message, "payload", payload);
    return message;
}

json_t *protocol_parse(const char *line, char *error, size_t error_size) {
    if (!line) {
        if (error && error_size > 0) {
            snprintf(error, error_size, "empty message");
        }
        return NULL;
    }

    json_error_t json_error;
    json_t *message = json_loads(line, 0, &json_error);
    if (!message) {
        if (error && error_size > 0) {
            snprintf(error, error_size, "invalid JSON: %s", json_error.text);
        }
        return NULL;
    }
    if (!json_is_object(message)) {
        if (error && error_size > 0) {
            snprintf(error, error_size, "message must be a JSON object");
        }
        json_decref(message);
        return NULL;
    }
    if (!protocol_validate_version(message)) {
        if (error && error_size > 0) {
            snprintf(error, error_size, "unsupported protocol version");
        }
        json_decref(message);
        return NULL;
    }
    if (!protocol_get_string(message, "type")) {
        if (error && error_size > 0) {
            snprintf(error, error_size, "missing message type");
        }
        json_decref(message);
        return NULL;
    }
    if (!protocol_get_payload(message)) {
        if (error && error_size > 0) {
            snprintf(error, error_size, "payload must be an object");
        }
        json_decref(message);
        return NULL;
    }

    return message;
}

const char *protocol_get_string(json_t *object, const char *key) {
    if (!object || !key) {
        return NULL;
    }
    json_t *value = json_object_get(object, key);
    return json_is_string(value) ? json_string_value(value) : NULL;
}

json_t *protocol_get_payload(json_t *message) {
    json_t *payload = json_object_get(message, "payload");
    return json_is_object(payload) ? payload : NULL;
}

bool protocol_validate_version(json_t *message) {
    json_t *version = json_object_get(message, "v");
    return json_is_integer(version) &&
           json_integer_value(version) == PROTOCOL_VERSION;
}

static void set_error(char *error, size_t error_size, const char *message) {
    if (error && error_size > 0) {
        snprintf(error, error_size, "%s", message);
    }
}

static bool has_integer(json_t *payload, const char *key) {
    return json_is_integer(json_object_get(payload, key));
}

bool protocol_is_known_client_type(const char *type) {
    return type &&
           (strcmp(type, "join") == 0 ||
            strcmp(type, "action") == 0 ||
            strcmp(type, "table_command") == 0 ||
            strcmp(type, "chat") == 0 ||
            strcmp(type, "ping") == 0);
}

bool protocol_validate_client_message(json_t *message, bool require_session,
                                      char *error, size_t error_size) {
    if (!message) {
        set_error(error, error_size, "missing message");
        return false;
    }

    const char *id = protocol_get_string(message, "id");
    const char *type = protocol_get_string(message, "type");
    json_t *payload = protocol_get_payload(message);

    if (!id || !id[0]) {
        set_error(error, error_size, "missing message id");
        return false;
    }
    if (!protocol_is_known_client_type(type)) {
        set_error(error, error_size, "unknown message type");
        return false;
    }
    if (!payload) {
        set_error(error, error_size, "payload must be an object");
        return false;
    }
    if (require_session && strcmp(type, "join") != 0 && strcmp(type, "ping") != 0) {
        const char *session = protocol_get_string(message, "session");
        if (!session || !session[0]) {
            set_error(error, error_size, "missing session");
            return false;
        }
    }

    if (strcmp(type, "join") == 0) {
        const char *name = protocol_get_string(payload, "name");
        if (!name || !name[0]) {
            set_error(error, error_size, "join requires name");
            return false;
        }
    } else if (strcmp(type, "action") == 0) {
        const char *action = protocol_get_string(payload, "action");
        if (!action || !action[0]) {
            set_error(error, error_size, "action requires action");
            return false;
        }
        if (strcmp(action, "fold") != 0 && strcmp(action, "check") != 0 &&
            strcmp(action, "call") != 0 && strcmp(action, "raise") != 0) {
            set_error(error, error_size, "unknown action");
            return false;
        }
        if (!has_integer(payload, "amount")) {
            set_error(error, error_size, "action requires integer amount");
            return false;
        }
    } else if (strcmp(type, "table_command") == 0) {
        const char *command = protocol_get_string(payload, "command");
        if (!command || !command[0]) {
            set_error(error, error_size, "table_command requires command");
            return false;
        }
        if (strcmp(command, "sitout") != 0 && strcmp(command, "sitin") != 0 &&
            strcmp(command, "rebuy") != 0) {
            set_error(error, error_size, "unknown table command");
            return false;
        }
        if (!has_integer(payload, "amount")) {
            set_error(error, error_size, "table_command requires integer amount");
            return false;
        }
    } else if (strcmp(type, "chat") == 0) {
        if (!protocol_get_string(payload, "text")) {
            set_error(error, error_size, "chat requires text");
            return false;
        }
    }

    return true;
}
