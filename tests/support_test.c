#include "audit.h"
#include "ids.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_ids(void) {
    char uuid[UUID_STRING_LEN];
    char token[65];

    assert(generate_uuid_string(uuid, sizeof(uuid)));
    assert(strlen(uuid) == 36);
    assert(generate_session_token(token, sizeof(token), 32));
    assert(strlen(token) == 64);
}

static void test_audit(void) {
    AuditLog audit;
    assert(audit_open(&audit, ":memory:"));
    assert(audit_count(&audit) == 0);
    assert(audit_append(&audit, 1, "state", "hand-1", "player-1",
                        "{\"type\":\"state\"}"));
    assert(audit_append(&audit, 2, "action", "hand-1", "player-1",
                        "{\"action\":\"call\"}"));
    assert(audit_count(&audit) == 2);
    audit_close(&audit);
}

int main(void) {
    test_ids();
    test_audit();
    printf("support: id and audit tests passed\n");
    return 0;
}
