# Quick Start

## Build

```bash
make deps-check
make
```

## Run a Local Game

Terminal 1:

```bash
bin/poker_server
```

Terminal 2:

```bash
bin/poker_client
```

Use defaults for local play:

- Server address: `127.0.0.1`
- Port: `5555`

Terminal 3:

```bash
bin/poker_client
```

The game starts automatically once two players are connected.

## In-Game Commands

- `fold`
- `check`
- `call`
- `raise 100`
- `chat Hello`
- `sitout`
- `sitin`
- `rebuy 1000`
- `help`
- `quit`

The client speaks the current newline-delimited JSON protocol. Manual protocol clients must send `join` first, then use the `session` returned in `welcome` for gameplay commands.

## Smoke Test

```bash
make smoke
```

This starts a temporary local server and two scripted clients. It may require local socket permission in sandboxed environments.

For the full protocol and gameplay checks:

```bash
make test
```

## Cleanup

```bash
make clean
```
