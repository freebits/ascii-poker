# Live Testing Guide

This guide is for a private LAN or VPS demo with invited testers. It is not a real-money, public-internet, or casino-grade deployment guide.

## Preflight

Install dependencies:

```bash
sudo apt-get install build-essential pkg-config libjansson-dev uuid-dev libssl-dev libsqlite3-dev
```

Build and test:

```bash
make clean
make deps-check
make all
make test
make smoke
```

Choose a server host that testers can reach. For a VPS, allow the selected TCP port in the firewall or security group. Keep access limited to trusted testers where possible.

## Start the Server

Local-only:

```bash
bin/poker_server --host 127.0.0.1 --port 5555 --audit-db poker_audit.sqlite3
```

LAN/VPS:

```bash
bin/poker_server --host 0.0.0.0 --port 5555 --audit-db poker_audit.sqlite3 --hand-delay 2
```

Useful options:

- `--host ADDR`: bind address, default `0.0.0.0`
- `--port PORT`: bind port, default `5555`
- `--audit-db PATH`: SQLite audit database path
- `--hand-delay SECONDS`: delay before the next hand starts after a result
- `--help`: print server usage

The server logs joins, leaves, actions, hand starts, hand results, protocol errors, and shutdown events to stdout.

## Connect Testers

Interactive:

```bash
bin/poker_client
```

Non-interactive startup:

```bash
bin/poker_client --name Alice --host 203.0.113.10 --port 5555
```

Client options:

- `--name NAME`: skip the player-name prompt
- `--host ADDR`: skip the server-address prompt
- `--port PORT`: skip the port prompt
- `--help`: print client usage

## Manual Test Script

Run through this with two to four testers:

1. Two players connect and confirm the first hand starts.
2. Current player folds and the other player wins.
3. Two players reach showdown by calling/checking through all streets.
4. Three players join; one raises, others call, and the game reaches the flop.
5. Send chat from one client and confirm the other clients receive it.
6. Use `sitout`, wait for the next hand, then use `sitin`.
7. Use `rebuy 1000` and confirm stack changes.
8. Type `ping` and confirm `PONG`.
9. Disconnect the current player during a hand and confirm the remaining player receives a hand result.
10. Stop the server with `Ctrl-C` and confirm it logs shutdown.

## Known Limits

- TCP is plain text. Use a private network, VPN, SSH tunnel, or trusted firewall rules for live tests.
- Sessions are table-session checks, not real authentication.
- Audit output is local SQLite and is not a compliance ledger.
- Reconnect creates a new seat; seat reservation/rejoin is not implemented.
- This build has no cashier, account ledger, anti-fraud, or payments.
