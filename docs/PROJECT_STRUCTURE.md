# Project Structure

```text
ascii-poker/
├── AGENTS.md              # Instructions and project notes for coding agents
├── Makefile               # Build, clean, run, and smoke-test targets
├── README.md              # Main user-facing project documentation
├── QUICKSTART.md          # Short local run instructions
├── include/
│   └── poker.h            # Shared card, deck, and hand-evaluation API
├── src/
│   ├── poker.c            # Core poker logic
│   ├── server.c           # Threaded TCP game server
│   └── client.c           # ASCII terminal client
├── tests/
│   └── smoke_two_clients.sh
├── scripts/               # Demo and manual helper scripts
└── docs/                  # Historical design and UI notes
```

## Generated Paths

These paths are created by `make` and ignored by git:

- `bin/`
- `build/`
- `*.log`
- temporary local test programs

## Module Dependencies

- `src/poker.c` is independent shared logic.
- `src/server.c` depends on `include/poker.h` and `src/poker.c`.
- `src/client.c` depends on `include/poker.h` and `src/poker.c`.

## Build Outputs

After `make`:

- `bin/poker_server`
- `bin/poker_client`
