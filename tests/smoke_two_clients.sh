#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVER="$ROOT_DIR/bin/poker_server"
CLIENT="$ROOT_DIR/bin/poker_client"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/ascii-poker-smoke.XXXXXX")"

server_pid=""
client1_pid=""
client2_pid=""

cleanup() {
    for pid in "$client1_pid" "$client2_pid" "$server_pid"; do
        if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done
    wait "$client1_pid" "$client2_pid" "$server_pid" 2>/dev/null || true
    rm -rf "$TMP_DIR"
}

trap cleanup EXIT

if [[ ! -x "$SERVER" || ! -x "$CLIENT" ]]; then
    echo "smoke: missing binaries; run make all first" >&2
    exit 1
fi

"$SERVER" >"$TMP_DIR/server.log" 2>&1 &
server_pid=$!
sleep 1

if ! kill -0 "$server_pid" 2>/dev/null; then
    echo "smoke: server failed to start" >&2
    sed -n '1,80p' "$TMP_DIR/server.log" >&2
    exit 1
fi

{
    sleep 12
    printf 'chat\nraise nope\nstatus\nping\ncall\nleave\n'
} | timeout 20 "$CLIENT" --name Alice --host 127.0.0.1 --port 5555 >"$TMP_DIR/client1.out" 2>"$TMP_DIR/client1.err" &
client1_pid=$!

sleep 1

{
    sleep 12
    printf 'bet 20\nallin\nstatus\nping\ncall\nleave\n'
} | timeout 20 "$CLIENT" --name Bob --host 127.0.0.1 --port 5555 >"$TMP_DIR/client2.out" 2>"$TMP_DIR/client2.err" &
client2_pid=$!

wait "$client1_pid" || true
client1_pid=""
wait "$client2_pid" || true
client2_pid=""

if ! grep -q 'Connected to 127.0.0.1:5555' "$TMP_DIR/client1.out"; then
    echo "smoke: client 1 did not connect" >&2
    sed -n '1,120p' "$TMP_DIR/client1.out" >&2
    sed -n '1,80p' "$TMP_DIR/client1.err" >&2
    exit 1
fi

if ! grep -q 'Connected to 127.0.0.1:5555' "$TMP_DIR/client2.out"; then
    echo "smoke: client 2 did not connect" >&2
    sed -n '1,120p' "$TMP_DIR/client2.out" >&2
    sed -n '1,80p' "$TMP_DIR/client2.err" >&2
    exit 1
fi

if ! grep -q 'Pot:' "$TMP_DIR/client1.out"; then
    echo "smoke: client 1 did not receive game state" >&2
    sed -n '1,140p' "$TMP_DIR/client1.out" >&2
    sed -n '1,100p' "$TMP_DIR/client1.err" >&2
    exit 1
fi

if ! grep -q 'Pot:' "$TMP_DIR/client2.out"; then
    echo "smoke: client 2 did not receive game state" >&2
    sed -n '1,140p' "$TMP_DIR/client2.out" >&2
    sed -n '1,100p' "$TMP_DIR/client2.err" >&2
    exit 1
fi

if ! grep -q 'Usage: chat <message>' "$TMP_DIR/client1.out"; then
    echo "smoke: client did not reject blank chat" >&2
    sed -n '1,160p' "$TMP_DIR/client1.out" >&2
    exit 1
fi

if ! grep -q 'Usage: raise <amount>' "$TMP_DIR/client1.out"; then
    echo "smoke: client did not reject invalid raise text" >&2
    sed -n '1,160p' "$TMP_DIR/client1.out" >&2
    exit 1
fi

echo "smoke: two clients connected and received initial game state"
