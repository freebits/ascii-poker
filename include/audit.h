#ifndef AUDIT_H
#define AUDIT_H

#include <stdbool.h>
#include <sqlite3.h>

typedef struct {
    sqlite3 *db;
} AuditLog;

bool audit_open(AuditLog *audit, const char *path);
void audit_close(AuditLog *audit);
bool audit_append(AuditLog *audit, int sequence, const char *event_type,
                  const char *hand_id, const char *player_id,
                  const char *json_payload);
int audit_count(AuditLog *audit);

#endif /* AUDIT_H */
