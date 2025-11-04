# Quick Start Guide - C Version

## Compilation

```bash
make
```

## Running the Game

### Terminal 1 - Start Server
```bash
./poker_server
```

### Terminal 2 - First Player
```bash
./poker_client
# Enter name: Alice
# Server address: 127.0.0.1
# Port: 5555
```

### Terminal 3 - Second Player
```bash
./poker_client
# Enter name: Bob
# Server address: 127.0.0.1
# Port: 5555
```

## Game will start automatically when 2+ players join!

## In-Game Commands

- `fold` - Fold hand
- `check` - Check (if no bet)
- `call` - Call current bet
- `raise 100` - Raise to 100 chips
- `chat Hello!` - Send message
- `help` - Show commands
- `quit` - Exit game

## Network Play

### On Server Machine
```bash
./poker_server
# Note your IP address (e.g., 192.168.1.100)
```

### On Client Machines
```bash
./poker_client
# Enter server IP: 192.168.1.100
# Port: 5555
```

## Cleanup

```bash
make clean
```

## Troubleshooting

**Port already in use:**
```bash
# Find process using port 5555
lsof -i :5555
# Kill it
kill -9 <PID>
```

**Compilation errors:**
```bash
# Ensure you have gcc and pthread
gcc --version
# On Debian/Ubuntu
sudo apt-get install build-essential
```

**Can't connect:**
- Check firewall allows port 5555
- Verify server is running
- Use correct IP address (127.0.0.1 for local)
