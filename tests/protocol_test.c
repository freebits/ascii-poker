#include "protocol.h"

#include <assert.h>
#include <string.h>

static void assert_valid(const char *line, bool require_session) {
    char error[128];
    json_t *msg = protocol_parse(line, error, sizeof(error));
    assert(msg != NULL);
    assert(protocol_validate_client_message(msg, require_session,
                                            error, sizeof(error)));
    json_decref(msg);
}

static void assert_invalid(const char *line, bool require_session,
                           const char *expected_error) {
    char error[128];
    json_t *msg = protocol_parse(line, error, sizeof(error));
    assert(msg != NULL);
    assert(!protocol_validate_client_message(msg, require_session,
                                             error, sizeof(error)));
    assert(strstr(error, expected_error) != NULL);
    json_decref(msg);
}

int main(void) {
    json_t *msg = protocol_client_message("client-1", "session-1", "action",
                                          json_pack("{s:s, s:i}",
                                                    "action", "call",
                                                    "amount", 0));
    assert(msg != NULL);
    assert(protocol_validate_version(msg));
    assert(strcmp(protocol_get_string(msg, "type"), "action") == 0);
    assert(protocol_get_payload(msg) != NULL);
    json_decref(msg);

    char error[128];
    msg = protocol_parse("{\"v\":1,\"type\":\"ping\",\"payload\":{}}",
                         error, sizeof(error));
    assert(msg != NULL);
    json_decref(msg);

    msg = protocol_parse("{\"v\":2,\"type\":\"ping\",\"payload\":{}}",
                         error, sizeof(error));
    assert(msg == NULL);
    assert(strstr(error, "unsupported") != NULL);

    msg = protocol_parse("{bad json", error, sizeof(error));
    assert(msg == NULL);
    assert(strstr(error, "invalid JSON") != NULL);

    msg = protocol_parse("{\"v\":1,\"payload\":{}}", error, sizeof(error));
    assert(msg == NULL);
    assert(strstr(error, "type") != NULL);

    msg = protocol_parse("{\"v\":1,\"type\":\"ping\",\"payload\":[]}",
                         error, sizeof(error));
    assert(msg == NULL);
    assert(strstr(error, "payload") != NULL);

    assert_valid("{\"v\":1,\"id\":\"join-1\",\"type\":\"join\",\"payload\":{\"name\":\"Alice\"}}",
                 false);
    assert_valid("{\"v\":1,\"id\":\"act-1\",\"session\":\"s\",\"type\":\"action\",\"payload\":{\"action\":\"call\",\"amount\":0}}",
                 true);
    assert_valid("{\"v\":1,\"id\":\"cmd-1\",\"session\":\"s\",\"type\":\"table_command\",\"payload\":{\"command\":\"rebuy\",\"amount\":1000}}",
                 true);
    assert_valid("{\"v\":1,\"id\":\"chat-1\",\"session\":\"s\",\"type\":\"chat\",\"payload\":{\"text\":\"hello\"}}",
                 true);

    assert_invalid("{\"v\":1,\"id\":\"x\",\"type\":\"legacy\",\"payload\":{}}",
                   false, "unknown message type");
    assert_invalid("{\"v\":1,\"id\":\"act-2\",\"type\":\"action\",\"payload\":{\"action\":\"call\",\"amount\":0}}",
                   true, "missing session");
    assert_invalid("{\"v\":1,\"id\":\"act-3\",\"session\":\"s\",\"type\":\"action\",\"payload\":{\"amount\":0}}",
                   true, "requires action");
    assert_invalid("{\"v\":1,\"id\":\"act-4\",\"session\":\"s\",\"type\":\"action\",\"payload\":{\"action\":\"dance\",\"amount\":0}}",
                   true, "unknown action");
    assert_invalid("{\"v\":1,\"id\":\"cmd-2\",\"session\":\"s\",\"type\":\"table_command\",\"payload\":{\"command\":\"rebuy\"}}",
                   true, "integer amount");
    assert_invalid("{\"v\":1,\"id\":\"chat-2\",\"session\":\"s\",\"type\":\"chat\",\"payload\":{}}",
                   true, "chat requires text");

    return 0;
}
