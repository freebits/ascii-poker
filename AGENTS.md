# AGENTS.md

## Project Snapshot

ASCII Poker is a C-only prototype of multiplayer no-limit Texas Hold'em. It has:

- shared poker logic in `src/poker.c` and `include/poker.h`
- a pthread-based TCP server in `src/server.c`
- an ASCII terminal client in `src/client.c`
- Makefile-based builds into `bin/` and `build/`

The project is a serious-demo cash-game prototype: clients can connect, act in turn, advance through streets, fold, call, check, raise to a total bet, handle side pots, reach showdown, and start the next hand. It is not real-money software.

## Repository Layout

```text
src/        C source files
include/    Public project headers
tests/      Automated smoke/integration scripts
scripts/    Demo and manual helper scripts
docs/       Design notes and historical project documentation
bin/        Generated binaries, ignored
build/      Generated object files, ignored
```

Do not commit generated binaries, object files, logs, or temporary test programs.

## Build and Run

```bash
make              # build bin/poker_server and bin/poker_client
make clean        # remove bin/ and build/
make debug        # rebuild with debug symbols
make run-server   # run bin/poker_server
make run-client   # run bin/poker_client
make smoke        # run the two-client integration smoke test
make integration  # run scripted protocol gameplay tests
make test         # run rule checks and protocol gameplay tests
make deps-check   # verify planned system dependencies
```

The server defaults to `0.0.0.0:5555`. The client defaults to `127.0.0.1:5555`. Both support `--host`, `--port`, and `--help`; the client also supports `--name`, and the server supports `--audit-db`, `--hand-delay`, `--action-timeout`, `--starting-chips`, `--min-rebuy`, and `--max-rebuy`.

## Testing Expectations

Before finishing code changes:

```bash
make clean && make all
make smoke
make integration
make test
```

Protocol dependency work expects system packages: `libjansson-dev`, `uuid-dev`, `libssl-dev`, `libsqlite3-dev`, and `pkg-config`.

`make smoke` starts a local TCP server and two scripted clients. In sandboxed environments, local socket creation may require explicit approval. If socket tests cannot run, report that clearly and still run the build.

For logic changes in `src/poker.c`, prefer adding focused C test coverage rather than relying only on manual play.

## C Style

- Use C11 and keep the project warning-clean under `-Wall -Wextra`.
- Use 4-space indentation and K&R braces.
- Use `snake_case` for functions and variables.
- Use PascalCase for typedef names and UPPER_CASE for enum values/constants.
- Prefer fixed-size buffers only when bounds are enforced.
- Prefer `snprintf` over `sprintf`; avoid unbounded `strcat`.
- Keep comments short and useful. Do not narrate obvious assignments.

## Protocol Notes

The server and client communicate with newline-delimited JSON. Every message has `v`, `type`, and object `payload` fields. Client messages also include `id`; server messages include `seq`. The initial `join` is sessionless, and `welcome` returns the `player_id` and `session` used by later gameplay messages.

When changing protocol behavior, update `src/protocol.c`, `src/server.c`, `src/client.c`, `tests/integration_gameplay.sh`, and `docs/PROTOCOL.md` together.

## Known Risks

- Disconnects during active hands need more robust state recovery.
- Short all-in raises are allowed, but edge cases around action reopening need more multiway tests.
- There is local SQLite audit logging, but no account ledger, cashier, anti-fraud, or compliance layer.
- Protocol sessions are lightweight table-session checks, not authentication.

Do not paper over these as solved unless tests prove the behavior.

## Cleanup Rules

- Keep root documentation concise and current.
- Put historical notes in `docs/`.
- Put manual demos in `scripts/`.
- Put automated checks in `tests/`.
- Keep generated output under ignored paths or `/tmp`.
- If moving files, update Makefile targets and README/QUICKSTART paths in the same change.
