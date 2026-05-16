# JSON Protocol

ASCII Poker uses one compact JSON object per TCP line. The transport is plain TCP on port `5555`; this protocol is for local/demo play and is not encrypted.

## Envelope

Client messages:

```json
{"v":1,"id":"msg-1","session":"token","type":"action","payload":{}}
```

Server messages:

```json
{"v":1,"seq":1,"reply_to":"msg-1","type":"state","payload":{}}
```

- `v` is currently `1`.
- `id` is required on every client message.
- `session` is omitted only for the initial `join` and may be omitted for `ping`.
- `seq` is a server-side monotonic sequence number.
- `payload` is always an object.

## Client Messages

- `join`: `{"name":"Alice"}`. Must be first and has no session.
- `action`: `{"action":"fold|check|call|raise","amount":0}`. Requires session.
- `table_command`: `{"command":"sitout|sitin|rebuy","amount":0}`. Requires session.
- `chat`: `{"text":"hello"}`. Requires session.
- `ping`: `{}`. Returns `pong`.

## Server Messages

- `welcome`: contains `player_id`, `session`, `name`, and starting `chips`.
- `state`: current pot, current bet, round, board cards, private hand, visible players, and embedded legal actions.
- `legal_actions`: legal action summary for the receiving player.
- `event`: player joins/leaves, chat, and action announcements.
- `hand_result`: showdown or fold-win winners.
- `error`: structured protocol or game error.
- `info`: table command status text.
- `pong`: reply to `ping`.

## Example Flow

```json
{"v":1,"id":"join-1","type":"join","payload":{"name":"Alice"}}
{"v":1,"seq":1,"type":"welcome","payload":{"player_id":"...","session":"...","name":"Alice","chips":1000}}
{"v":1,"id":"act-1","session":"...","type":"action","payload":{"action":"call","amount":0}}
{"v":1,"seq":2,"type":"event","payload":{"event":"action","player":"Alice","action":"call","amount":10}}
```

## Audit Output

The server writes best-effort local audit events to `poker_audit.sqlite3`. That database is runtime output, ignored by git, and is not a compliance ledger.
