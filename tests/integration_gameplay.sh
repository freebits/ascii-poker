#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVER="$ROOT_DIR/bin/poker_server"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/ascii-poker-integration.XXXXXX")"
server_pid=""
server_port=5555
alice_session=""
bob_session=""
carol_session=""

cleanup() {
    exec 3<&- 3>&- 2>/dev/null || true
    exec 4<&- 4>&- 2>/dev/null || true
    exec 5<&- 5>&- 2>/dev/null || true
    if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
        kill "$server_pid" 2>/dev/null || true
    fi
    wait "$server_pid" 2>/dev/null || true
    rm -rf "$TMP_DIR"
}

trap cleanup EXIT

start_server() {
    server_port="${1:-5555}"
    shift || true
    "$SERVER" --host 127.0.0.1 --port "$server_port" --audit-db "$TMP_DIR/audit.sqlite3" --hand-delay 0 "$@" >"$TMP_DIR/server.log" 2>&1 &
    server_pid=$!
    sleep 1

    if ! kill -0 "$server_pid" 2>/dev/null; then
        echo "integration: server failed to start" >&2
        sed -n '1,120p' "$TMP_DIR/server.log" >&2
        exit 1
    fi
}

stop_server() {
    exec 3<&- 3>&- 2>/dev/null || true
    exec 4<&- 4>&- 2>/dev/null || true
    exec 5<&- 5>&- 2>/dev/null || true
    if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
        kill "$server_pid" 2>/dev/null || true
    fi
    wait "$server_pid" 2>/dev/null || true
    server_pid=""
}

connect_third_player() {
    local welcome
    exec 5<>/dev/tcp/127.0.0.1/"$server_port"
    printf '{"v":1,"id":"join-carol","type":"join","payload":{"name":"Carol"}}\n' >&5
    wait_for_type 5 "welcome" welcome
    carol_session="$(jq -r '.payload.session' <<<"$welcome")"
}

connect_players() {
    local welcome
    exec 3<>/dev/tcp/127.0.0.1/"$server_port"
    printf '{"v":1,"id":"join-alice","type":"join","payload":{"name":"Alice"}}\n' >&3
    wait_for_type 3 "welcome" welcome
    alice_session="$(jq -r '.payload.session' <<<"$welcome")"

    exec 4<>/dev/tcp/127.0.0.1/"$server_port"
    printf '{"v":1,"id":"join-bob","type":"join","payload":{"name":"Bob"}}\n' >&4
    wait_for_type 4 "welcome" welcome
    bob_session="$(jq -r '.payload.session' <<<"$welcome")"
}

field_after_prefix() {
    local state="$1"
    local index="$2"
    case "$index" in
        1) jq -r '.payload.pot' <<<"$state" ;;
        2) jq -r '.payload.current_bet' <<<"$state" ;;
        3) jq -r '.payload.round' <<<"$state" ;;
    esac
}

players_part() {
    jq -c '.payload.players' <<<"$1"
}

current_player() {
    jq -r '.payload.players[] | select(.current == true) | .name' <<<"$1"
}

player_bet() {
    jq -r --arg target "$2" '.payload.players[] | select(.name == $target) | .bet' <<<"$1"
}

wait_for_type() {
    local fd="$1"
    local type="$2"
    local __result="$3"
    local line line_type

    for _ in {1..30}; do
        if read -r -t 1 -u "$fd" line; then
            line_type="$(jq -r '.type // empty' <<<"$line" 2>/dev/null || true)"
            if [[ "$line_type" == "$type" ]]; then
                printf -v "$__result" '%s' "$line"
                return 0
            fi
        fi
    done

    echo "integration: timed out waiting for type '$type' on fd $fd" >&2
    return 1
}

wait_for_state() {
    local fd="$1"
    local __result="$2"
    local line

    for _ in {1..25}; do
        if read -r -t 1 -u "$fd" line; then
            if [[ "$(jq -r '.type // empty' <<<"$line" 2>/dev/null || true)" == "state" ]]; then
                printf -v "$__result" '%s' "$line"
                return 0
            fi
        fi
    done

    echo "integration: timed out waiting for state on fd $fd" >&2
    return 1
}

wait_for_contains() {
    local fd="$1"
    local pattern="$2"
    local line

    for _ in {1..30}; do
        if read -r -t 1 -u "$fd" line; then
            if [[ "$line" == *"$pattern"* ]]; then
                return 0
            fi
        fi
    done

    echo "integration: timed out waiting for '$pattern' on fd $fd" >&2
    return 1
}

wait_no_type() {
    local fd="$1"
    local type="$2"
    local line

    for _ in {1..3}; do
        if read -r -t 1 -u "$fd" line; then
            if [[ "$(jq -r '.type // empty' <<<"$line" 2>/dev/null || true)" == "$type" ]]; then
                echo "integration: unexpectedly received type '$type' on fd $fd" >&2
                return 1
            fi
        fi
    done
}

send_action_for_player() {
    local player="$1"
    local action="$2"
    local amount="${3:-0}"

    if [[ "$player" == "Alice" ]]; then
        printf '{"v":1,"id":"action-alice","session":"%s","type":"action","payload":{"action":"%s","amount":%s}}\n' "$alice_session" "$action" "$amount" >&3
    elif [[ "$player" == "Bob" ]]; then
        printf '{"v":1,"id":"action-bob","session":"%s","type":"action","payload":{"action":"%s","amount":%s}}\n' "$bob_session" "$action" "$amount" >&4
    else
        printf '{"v":1,"id":"action-carol","session":"%s","type":"action","payload":{"action":"%s","amount":%s}}\n' "$carol_session" "$action" "$amount" >&5
    fi
}

send_table_command_for_player() {
    local player="$1"
    local command="$2"
    local amount="${3:-0}"

    if [[ "$player" == "Alice" ]]; then
        printf '{"v":1,"id":"cmd-alice","session":"%s","type":"table_command","payload":{"command":"%s","amount":%s}}\n' "$alice_session" "$command" "$amount" >&3
    elif [[ "$player" == "Bob" ]]; then
        printf '{"v":1,"id":"cmd-bob","session":"%s","type":"table_command","payload":{"command":"%s","amount":%s}}\n' "$bob_session" "$command" "$amount" >&4
    else
        printf '{"v":1,"id":"cmd-carol","session":"%s","type":"table_command","payload":{"command":"%s","amount":%s}}\n' "$carol_session" "$command" "$amount" >&5
    fi
}

send_ping_for_player() {
    local player="$1"
    if [[ "$player" == "Alice" ]]; then
        printf '{"v":1,"id":"ping-alice","session":"%s","type":"ping","payload":{}}\n' "$alice_session" >&3
    else
        printf '{"v":1,"id":"ping-bob","session":"%s","type":"ping","payload":{}}\n' "$bob_session" >&4
    fi
}

test_custom_port_and_ping() {
    start_server 5566
    connect_players

    send_ping_for_player "Alice"
    wait_for_type 3 "pong" line

    stop_server
    echo "integration: custom_port_and_ping passed"
}

test_protocol_rejects_legacy_first_message() {
    start_server
    exec 3<>/dev/tcp/127.0.0.1/"$server_port"
    printf 'NAME:Alice\n' >&3
    wait_for_contains 3 "invalid JSON"
    stop_server
    echo "integration: protocol_rejects_legacy_first_message passed"
}

test_protocol_requires_join_first() {
    start_server
    exec 3<>/dev/tcp/127.0.0.1/"$server_port"
    printf '{"v":1,"id":"act-before-join","type":"action","payload":{"action":"call","amount":0}}\n' >&3
    wait_for_contains 3 "First message must be join"
    stop_server
    echo "integration: protocol_requires_join_first passed"
}

test_protocol_rejects_bad_session() {
    start_server
    connect_players

    printf '{"v":1,"id":"bad-session","session":"wrong","type":"action","payload":{"action":"call","amount":0}}\n' >&3
    wait_for_contains 3 "Invalid session"

    stop_server
    echo "integration: protocol_rejects_bad_session passed"
}

test_sitting_out_player_does_not_start_hand() {
    start_server
    local welcome

    exec 3<>/dev/tcp/127.0.0.1/"$server_port"
    printf '{"v":1,"id":"join-alice","type":"join","payload":{"name":"Alice"}}\n' >&3
    wait_for_type 3 "welcome" welcome
    alice_session="$(jq -r '.payload.session' <<<"$welcome")"

    send_table_command_for_player "Alice" "sitout" 0
    wait_for_contains 3 "sitting out"

    exec 4<>/dev/tcp/127.0.0.1/"$server_port"
    printf '{"v":1,"id":"join-bob","type":"join","payload":{"name":"Bob"}}\n' >&4
    wait_for_type 4 "welcome" welcome
    bob_session="$(jq -r '.payload.session' <<<"$welcome")"

    wait_no_type 4 "state"

    stop_server
    echo "integration: sitting_out_player_does_not_start_hand passed"
}

test_rebuy_limits() {
    start_server 5555 --min-rebuy 100 --max-rebuy 1000
    connect_players

    send_table_command_for_player "Alice" "rebuy" 50
    wait_for_contains 3 "Rebuy below table minimum"

    send_table_command_for_player "Alice" "rebuy" 1500
    wait_for_contains 3 "Rebuy above table maximum"

    stop_server
    echo "integration: rebuy_limits passed"
}

test_protocol_error_limit_disconnects_client() {
    start_server
    connect_players

    for _ in {1..3}; do
        printf '{"v":1,"id":"bad","session":"%s","type":"action","payload":{"action":"dance","amount":0}}\n' "$alice_session" >&3
    done

    local line disconnected=0
    for _ in {1..10}; do
        if ! read -r -t 1 -u 3 line; then
            disconnected=1
            break
        fi
    done

    if [[ "$disconnected" != "1" ]]; then
        echo "integration: expected client disconnect after protocol errors" >&2
        exit 1
    fi

    stop_server
    echo "integration: protocol_error_limit_disconnects_client passed"
}

test_disconnect_during_active_hand() {
    start_server
    connect_players

    local state current
    wait_for_state 3 state
    current="$(current_player "$state")"

    if [[ "$current" == "Alice" ]]; then
        exec 3<&- 3>&-
        wait_for_type 4 "hand_result" line
    else
        exec 4<&- 4>&-
        wait_for_type 3 "hand_result" line
    fi

    stop_server
    echo "integration: disconnect_during_active_hand passed"
}

test_timeout_auto_folds_when_facing_bet() {
    start_server 5555 --action-timeout 1
    connect_players

    wait_for_type 3 "hand_result" line

    stop_server
    echo "integration: timeout_auto_folds_when_facing_bet passed"
}

test_timeout_auto_checks_when_possible() {
    start_server 5555 --action-timeout 1
    connect_players

    local state current
    wait_for_state 3 state
    current="$(current_player "$state")"
    send_action_for_player "$current" "call" 0

    wait_for_contains 3 '"event":"timeout"'
    wait_for_contains 3 '"action":"check"'

    stop_server
    echo "integration: timeout_auto_checks_when_possible passed"
}

test_fold_win() {
    start_server
    connect_players

    local state current
    wait_for_state 3 state
    current="$(current_player "$state")"
    send_action_for_player "$current" "fold" 0

    wait_for_type 3 "hand_result" line
    stop_server
    echo "integration: fold_win passed"
}

test_invalid_actions() {
    start_server
    connect_players

    local state current non_current
    wait_for_state 3 state
    current="$(current_player "$state")"
    if [[ "$current" == "Alice" ]]; then
        non_current="Bob"
    else
        non_current="Alice"
    fi

    send_action_for_player "$non_current" "call" 0
    if [[ "$non_current" == "Alice" ]]; then
        wait_for_contains 3 "Not your turn"
    else
        wait_for_contains 4 "Not your turn"
    fi

    send_action_for_player "$current" "check" 0
    if [[ "$current" == "Alice" ]]; then
        wait_for_contains 3 "Cannot check"
    else
        wait_for_contains 4 "Cannot check"
    fi

    send_action_for_player "$current" "raise" 15
    if [[ "$current" == "Alice" ]]; then
        wait_for_contains 3 "Minimum raise is 20"
    else
        wait_for_contains 4 "Minimum raise is 20"
    fi

    send_action_for_player "$current" "fold" 0
    wait_for_type 3 "hand_result" line
    stop_server
    echo "integration: invalid_actions passed"
}

test_call_check_showdown() {
    start_server
    connect_players

    local state line current current_bet bet key last_key=""
    wait_for_state 3 state

    for _ in {1..40}; do
        while read -r -t 0.1 -u 3 line; do
            [[ "$(jq -r '.type // empty' <<<"$line" 2>/dev/null || true)" == "hand_result" ]] && {
                stop_server
                echo "integration: call_check_showdown passed"
                return
            }
            [[ "$(jq -r '.type // empty' <<<"$line" 2>/dev/null || true)" == "state" ]] && state="$line"
        done

        while read -r -t 0.1 -u 4 line; do
            [[ "$(jq -r '.type // empty' <<<"$line" 2>/dev/null || true)" == "hand_result" ]] && {
                stop_server
                echo "integration: call_check_showdown passed"
                return
            }
            [[ "$(jq -r '.type // empty' <<<"$line" 2>/dev/null || true)" == "state" ]] && state="$line"
        done

        current="$(current_player "$state")"
        [[ -z "$current" ]] && continue

        current_bet="$(field_after_prefix "$state" 2)"
        bet="$(player_bet "$state" "$current")"
        key="$(field_after_prefix "$state" 3):$(field_after_prefix "$state" 1):$current:$current_bet:$bet"

        if [[ "$key" == "$last_key" ]]; then
            sleep 0.2
            continue
        fi
        last_key="$key"

        if (( current_bet > bet )); then
            send_action_for_player "$current" "call" 0
        else
            send_action_for_player "$current" "check" 0
        fi
    done

    echo "integration: call_check_showdown did not reach hand_result" >&2
    exit 1
}

test_three_player_raise_flow() {
    start_server
    connect_players
    connect_third_player

    local state current
    wait_for_state 3 state

    for _ in {1..10}; do
        current="$(current_player "$state")"
        case "$current" in
            Alice|Bob|Carol)
                if [[ "$(field_after_prefix "$state" 2)" == "10" ]]; then
                    send_action_for_player "$current" "raise" 40
                else
                    send_action_for_player "$current" "call" 0
                fi
                ;;
        esac

        wait_for_state 3 state
        if [[ "$(field_after_prefix "$state" 3)" == "flop" ]]; then
            stop_server
            echo "integration: three_player_raise_flow passed"
            return
        fi
    done

    echo "integration: three_player_raise_flow did not reach flop" >&2
    exit 1
}

if [[ ! -x "$SERVER" ]]; then
    echo "integration: missing server binary; run make all first" >&2
    exit 1
fi

test_custom_port_and_ping
test_protocol_rejects_legacy_first_message
test_protocol_requires_join_first
test_protocol_rejects_bad_session
test_sitting_out_player_does_not_start_hand
test_rebuy_limits
test_protocol_error_limit_disconnects_client
test_disconnect_during_active_hand
test_timeout_auto_folds_when_facing_bet
test_timeout_auto_checks_when_possible
test_fold_win
test_invalid_actions
test_call_check_showdown
test_three_player_raise_flow
