# ASCII Texas Hold'em Poker

A C implementation of a multiplayer no-limit Texas Hold'em poker game with a pure ASCII terminal interface. The project currently provides a threaded TCP server, a terminal client, and shared poker logic for cards, decks, shuffling, and hand evaluation.

## Current State

This is a serious-demo cash-game prototype. It can build, start a local server, connect multiple clients, progress hands through betting rounds, resolve fold wins, handle side pots, reach showdown, and start the next hand. Core poker hand evaluation and a line-delimited JSON protocol are implemented, but several areas still need hardening before treating it as real-money software:

- The JSON protocol is versioned and tested, but is not encrypted or internet-hardened.
- Disconnect recovery is basic and should be tested more deeply with multi-player hands.
- There is local audit-event storage, but no account ledger, cashier, compliance, or anti-fraud layer.

See [AGENTS.md](AGENTS.md) for engineering notes and known risks.

## Requirements

- GCC or a compatible C compiler
- POSIX sockets and pthreads
- Linux, macOS, BSD, or another POSIX-like environment
- `make`
- `pkg-config`
- Development packages:
  - `libjansson-dev` for JSON protocol messages
  - `uuid-dev` for table/hand/session identifiers
  - `libssl-dev` for secure random session tokens
  - `libsqlite3-dev` for local audit/event storage

## Build

```bash
make
```

Build output is written to:

- `bin/poker_server`
- `bin/poker_client`

Useful targets:

```bash
make all          # Build server and client
make server       # Build only the server
make client       # Build only the client
make debug        # Rebuild with debug symbols
make clean        # Remove bin/ and build/
make smoke        # Run the local two-client smoke test
make integration  # Run scripted gameplay integration tests
make test         # Run rules and gameplay tests
make deps-check   # Verify planned system dependencies
```

## Run

Start the server:

```bash
bin/poker_server
```

For a LAN/VPS live test:

```bash
bin/poker_server --host 0.0.0.0 --port 5555 --audit-db poker_audit.sqlite3 --hand-delay 2 --action-timeout 60
```

In another terminal, start a client:

```bash
bin/poker_client
```

Or skip prompts:

```bash
bin/poker_client --name Alice --host 127.0.0.1 --port 5555
```

The client prompts for:

1. Player name
2. Server address, default `127.0.0.1`
3. Server port, default `5555`

Start a second client to trigger the first hand.

## Commands

During a game, the client accepts:

- `fold`
- `check`
- `call`
- `raise <amount>`
- `chat <message>`
- `sitout`
- `sitin`
- `rebuy [amount]`
- `ping`
- `status`
- `help`
- `quit` / `leave`

## Test

Run the integration smoke test:

```bash
make smoke
```

The smoke test starts one local server and two scripted clients, then checks that a hand state is received. It uses local TCP sockets, so sandboxed environments may require explicit permission for socket creation.

Run the gameplay integration suite:

```bash
make integration
```

The integration suite drives JSON protocol clients through fold-win, invalid-action, call/check-to-showdown, and three-player raise scenarios.

Run the full test suite:

```bash
make test
```

The full suite adds direct side-pot rule checks, protocol validation tests, and support-module tests.

Run dependency validation:

```bash
make deps-check
```

On Debian/Ubuntu, install planned protocol dependencies with:

```bash
sudo apt-get install libjansson-dev uuid-dev libssl-dev libsqlite3-dev pkg-config
```

## Layout

```text
.
├── AGENTS.md
├── Makefile
├── README.md
├── QUICKSTART.md
├── docs/
├── include/
│   ├── audit.h
│   ├── ids.h
│   ├── poker.h
│   └── protocol.h
├── scripts/
├── src/
│   ├── audit.c
│   ├── client.c
│   ├── ids.c
│   ├── poker.c
│   ├── protocol.c
│   └── server.c
└── tests/
    ├── integration_gameplay.sh
    ├── protocol_test.c
    ├── rules_test.c
    ├── smoke_two_clients.sh
    └── support_test.c
```

Generated files are ignored and should not be committed. The server writes local audit events to `poker_audit.sqlite3`; that runtime database is also ignored.

## Protocol

The TCP protocol is newline-delimited JSON. Every client message uses:

```json
{"v":1,"id":"msg-1","session":"token-after-welcome","type":"action","payload":{"action":"call","amount":0}}
```

The initial `join` message is the only gameplay message sent without a `session`. The server replies with `welcome`, including `player_id` and `session`, then requires that session on `action`, `table_command`, and `chat` messages.

See `docs/PROTOCOL.md` for the protocol reference.

For private demo operations, see `docs/LIVE_TESTING.md`.
