#include "audit.h"

#include <stddef.h>

bool audit_open(AuditLog *audit, const char *path) {
    if (!audit || !path) {
        return false;
    }

    audit->db = NULL;
    if (sqlite3_open(path, &audit->db) != SQLITE_OK) {
        audit_close(audit);
        return false;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS audit_events ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "sequence INTEGER NOT NULL,"
        "event_type TEXT NOT NULL,"
        "hand_id TEXT,"
        "player_id TEXT,"
        "payload TEXT NOT NULL"
        ")";

    if (sqlite3_exec(audit->db, sql, NULL, NULL, NULL) != SQLITE_OK) {
        audit_close(audit);
        return false;
    }

    return true;
}

void audit_close(AuditLog *audit) {
    if (audit && audit->db) {
        sqlite3_close(audit->db);
        audit->db = NULL;
    }
}

bool audit_append(AuditLog *audit, int sequence, const char *event_type,
                  const char *hand_id, const char *player_id,
                  const char *json_payload) {
    if (!audit || !audit->db || !event_type || !json_payload) {
        return false;
    }

    const char *sql =
        "INSERT INTO audit_events "
        "(sequence, event_type, hand_id, player_id, payload) "
        "VALUES (?, ?, ?, ?, ?)";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(audit->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, sequence);
    sqlite3_bind_text(stmt, 2, event_type, -1, SQLITE_TRANSIENT);
    if (hand_id) {
        sqlite3_bind_text(stmt, 3, hand_id, -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }
    if (player_id) {
        sqlite3_bind_text(stmt, 4, player_id, -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 4);
    }
    sqlite3_bind_text(stmt, 5, json_payload, -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

int audit_count(AuditLog *audit) {
    if (!audit || !audit->db) {
        return -1;
    }

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(audit->db, "SELECT COUNT(*) FROM audit_events",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    int count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}
