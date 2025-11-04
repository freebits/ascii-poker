# Suggested Commands

## Build Commands
```bash
# Build both server and client
make all

# Clean build artifacts
make clean

# Build with debug symbols
make debug

# Clean and rebuild
make clean && make all
```

## Run Commands
```bash
# Start the poker server (default port 8080)
./poker_server

# Start a poker client (connects to localhost:8080)
./poker_client

# Run with debug output (stderr)
./poker_client 2>&1 | tee client_debug.log

# Alternative: Using make targets
make run-server    # Start server
make run-client    # Start client
```

## Installation Commands
```bash
# Install to /usr/local/bin (requires sudo)
sudo make install

# Uninstall
sudo make uninstall
```

## Testing/Debugging
```bash
# Run automated test with debug capture
./test_debug.sh

# Check for memory leaks with valgrind (if installed)
valgrind --leak-check=full ./poker_server
valgrind --leak-check=full ./poker_client

# Run multiple clients for testing
# Terminal 1:
./poker_server

# Terminal 2:
./poker_client 2>client1.log

# Terminal 3:
./poker_client 2>client2.log
```

## Development Tools
```bash
# Check code with static analyzer
gcc -Wall -Wextra -Wconversion -Wshadow -pedantic -c server.c
gcc -Wall -Wextra -Wconversion -Wshadow -pedantic -c client.c
gcc -Wall -Wextra -Wconversion -Wshadow -pedantic -c poker.c

# Format code (if clang-format available)
clang-format -i *.c *.h
```

## System Commands (Linux)
- `ls -lh` - List files with human-readable sizes
- `ps aux | grep poker` - Find running poker processes
- `netstat -an | grep 8080` - Check if port 8080 is in use
- `kill <PID>` - Stop a process
- `pkill poker_server` - Kill all server processes
