# ASCII Texas Hold'em Poker

A multiplayer no-limit Texas Hold'em poker game with a **pure ASCII terminal interface**. Features a client-server architecture allowing multiple players to compete in a single game.

**Now available in both Python and C implementations!**

## Features

- **No-Limit Texas Hold'em**: Classic poker variant with community cards
- **Multiplayer**: Support for multiple players in the same game (up to 10)
- **Pure ASCII Interface**: Clean, terminal-based UI using only standard ASCII characters
  - Works on ANY terminal (no UTF-8 required)
  - Perfect for SSH sessions
  - Screen reader compatible
  - Clear visual hierarchy with `>>>` markers, `[DEALER]` labels, and organized sections
- **Complete Poker Logic**: 
  - Hand evaluation (Royal Flush down to High Card)
  - Betting rounds (Preflop, Flop, Turn, River)
  - Blinds system (5/10)
  - All-in support
  - Split pots for ties
- **Client-Server Architecture**: Separate server and client applications
- **Real-time Updates**: See other players' actions as they happen
- **Chat System**: Communicate with other players at the table
- **Multi-threaded**: Handles multiple concurrent clients efficiently
- **Cryptographically Secure RNG**: Uses /dev/urandom with rejection sampling

## Requirements

### C Version (Recommended for Performance)
- GCC compiler or compatible C compiler
- POSIX-compliant system (Linux, macOS, BSD)
- pthread library (usually included)
- Standard C library

### Python Version
- Python 3.7 or higher
- Standard library only (no external dependencies)

## Installation

### C Version

1. Clone or download the repository:
```bash
cd ascii-poker
```

2. Compile the project:
```bash
make
```

This will create two executables:
- `poker_server` - The game server
- `poker_client` - The client application

**Compilation Options:**
```bash
make all          # Build both server and client (default)
make clean        # Remove compiled files
make debug        # Build with debug symbols
make run-server   # Build and run the server
make run-client   # Build and run the client
```

### Python Version

No installation needed - uses Python standard library only.

```bash
cd ascii-poker
```

## Usage

### C Version

**Starting the Server:**

```bash
./poker_server
```

The server listens on `0.0.0.0:5555` by default.

**Connecting as a Client:**

```bash
./poker_client
```

You'll be prompted for:
1. **Your name**: Enter your player name
2. **Server address**: Enter the server IP (default: 127.0.0.1 for local)
3. **Server port**: Enter the port (default: 5555)

### Python Version

**Starting the Server:**

Run the server first to host the game:

```bash
python server.py
```

By default, the server listens on `0.0.0.0:5555` (all interfaces, port 5555).

The server will:
- Wait for at least 2 players to join
- Automatically start hands when enough players are present
- Handle disconnections gracefully
- Manage all game logic and state

**Connecting as a Client:**

In a separate terminal (or on a different machine), run the client:

```bash
python client.py
```

You'll be prompted for:
1. **Your name**: Enter your player name
2. **Server address**: Enter the server IP (default: 127.0.0.1 for local)
3. **Server port**: Enter the port (default: 5555)

## How to Play

### Game Flow

1. **Joining**: Connect to the server and wait for other players
2. **Starting Chips**: Each player starts with 1000 chips
3. **Blinds**: Small blind (5) and big blind (10) are posted automatically
4. **Hole Cards**: You receive 2 private cards
5. **Betting Rounds**:
   - **Preflop**: After receiving hole cards
   - **Flop**: After 3 community cards are dealt
   - **Turn**: After 4th community card
   - **River**: After 5th community card
6. **Showdown**: Best hand wins the pot

### Commands

While playing, use these commands:

- `fold` - Fold your hand (forfeit the current hand)
- `check` - Pass without betting (only if no bet to call)
- `call` - Match the current bet
- `raise <amount>` - Raise to a specific amount (e.g., `raise 50`)
- `chat <message>` - Send a message to other players (e.g., `chat good hand!`)
- `help` - Show available commands
- `quit` - Leave the game

### Card Notation

Cards are displayed in standard notation:
- **Ranks**: 2, 3, 4, 5, 6, 7, 8, 9, T (10), J (Jack), Q (Queen), K (King), A (Ace)
- **Suits**: C (Clubs), D (Diamonds), H (Hearts), S (Spades)

Example: `[AH]` = Ace of Hearts, `[TD]` = Ten of Diamonds

### Hand Rankings (Highest to Lowest)

1. **Royal Flush**: A-K-Q-J-T all same suit
2. **Straight Flush**: Five consecutive cards same suit
3. **Four of a Kind**: Four cards of same rank
4. **Full House**: Three of a kind + a pair
5. **Flush**: Five cards same suit
6. **Straight**: Five consecutive cards
7. **Three of a Kind**: Three cards of same rank
8. **Two Pair**: Two different pairs
9. **One Pair**: Two cards of same rank
10. **High Card**: Highest single card

## Example Game Session

```
======================================================================
                         TEXAS HOLD'EM
======================================================================

POT: 150 chips | CURRENT BET: 50 chips | ROUND: FLOP

COMMUNITY CARDS: [AH] [KD] [QS]

----------------------------------------------------------------------

PLAYERS:
  Alice: 950 chips (bet: 50) [DEALER]
  Bob: 900 chips (bet: 50) 
>> Charlie: 925 chips (bet: 50) << [*** CURRENT TURN ***]
  Dave: 850 chips (bet: 0) 

----------------------------------------------------------------------

YOUR HAND: [AS] [KH]

YOUR CHIPS: 925 | YOUR BET THIS ROUND: 50

======================================================================
YOUR TURN! Available actions:
  fold    - Fold your hand
  check   - Check (if no bet to call)
  call    - Call the current bet
  raise X - Raise to X chips (min: 100)
======================================================================

Type 'help' for commands | Type 'quit' to exit

> raise 100
```

## Network Configuration

### Playing on Different Machines

1. **Server**: Note the server's IP address
   ```bash
   # On server machine
   python server.py
   ```

2. **Clients**: Connect using server's IP
   ```bash
   # On client machine
   python client.py
   # When prompted, enter server's IP address
   ```

### Firewall Settings

Ensure port 5555 (or your chosen port) is open for TCP connections.

## Architecture

### C Version Files

- **`poker.h`**: Header file with data structures and function declarations
- **`poker.c`**: Core poker logic (cards, deck, hand evaluation)
- **`server.c`**: Multi-threaded game server with pthread support
- **`client.c`**: Terminal-based client with threaded message handling
- **`Makefile`**: Build system for easy compilation
- **`README.md`**: This file

### Python Version Files

- **`poker.py`**: Core poker logic (cards, deck, hand evaluation)
- **`server.py`**: Game server managing state and multiple clients
- **`client.py`**: Terminal-based client with ASCII UI
- **`README.md`**: This file

### Server Components

- **PokerServer/Game Server**: Handles network connections and game state
- **PokerGame/Game Logic**: Manages game rules, betting, and hand progression
- **Player Management**: Tracks player states, chips, and actions
- **Threading**: Separate threads for each client connection

### Client Components

- **PokerClient/Client**: Handles server communication
- **Display Functions**: ASCII UI rendering
- **Input Handler**: Command processing
- **Receive Thread**: Asynchronous message handling

## Troubleshooting

### Connection Issues

- Ensure server is running before connecting clients
- Check firewall settings allow port 5555
- Verify IP address is correct (use `127.0.0.1` for local play)

### Game Not Starting

- Need at least 2 players with chips to start a hand
- Wait for other players to join

### Disconnections

- Server handles player disconnections gracefully
- Disconnected players are removed from current hand
- Game continues with remaining players

## Advanced Features

### Custom Blind Levels

**C Version:**
Edit `server.c`:
```c
#define SMALL_BLIND 5   // Change this value
#define BIG_BLIND 10    // Change this value
```

**Python Version:**
Edit `server.py`:
```python
self.small_blind = 5   # Change this value
self.big_blind = 10    # Change this value
```

### Starting Chip Stack

**C Version:**
Edit `server.c`:
```c
#define STARTING_CHIPS 1000  // Change starting chips
```

**Python Version:**
Edit `player.py` Player class:
```python
self.chips = 1000  # Change starting chips
```

### Custom Port

**C Version:**
Edit `server.c`:
```c
#define PORT 5555  // Change port number
```

**Python Version:**
Run server on different port:
```python
server = PokerServer(port=8888)  # Edit in server.py
```

## Development

### C Version

**Compilation Flags:**
- `-Wall -Wextra`: Enable all warnings
- `-O2`: Optimization level 2
- `-pthread`: POSIX thread support
- `-std=c11`: C11 standard

**Debug Build:**
```bash
make debug
```

**Memory Checking (with Valgrind):**
```bash
valgrind --leak-check=full ./poker_server
valgrind --leak-check=full ./poker_client
```

**Code Structure:**
- All functions follow single responsibility principle
- Thread-safe operations using mutex locks
- Proper memory management (no leaks)
- Error handling for network operations

### Python Version

**Running Tests:**

The game includes comprehensive hand evaluation logic. To test manually:

```python
from poker import Card, HandEvaluator, Rank, Suit

# Create some cards
cards = [
    Card(Rank.ACE, Suit.HEARTS),
    Card(Rank.KING, Suit.HEARTS),
    Card(Rank.QUEEN, Suit.HEARTS),
    Card(Rank.JACK, Suit.HEARTS),
    Card(Rank.TEN, Suit.HEARTS)
]

# Evaluate
rank, tiebreakers = HandEvaluator.evaluate(cards)
print(HandEvaluator.hand_name(rank))  # Royal Flush
```

### Performance Comparison

**C Version:**
- ~100x faster hand evaluation
- Lower memory footprint
- Efficient thread management
- Suitable for production use

**Python Version:**
- Easier to modify and extend
- Good for prototyping
- Suitable for casual games

### Contributing

Feel free to extend the game with:
- Tournament mode
- Player statistics
- Hand history logging
- Replay functionality
- Better ASCII art for cards
- Configurable blinds structure
- Ante support
- Multiple tables

## License

Free to use and modify.

## Technical Details

### Protocol (C Version)

The C implementation uses a simple text-based protocol over TCP:

**Server → Client Messages:**
- `NAME_REQUEST` - Request player name
- `WELCOME:name:chips` - Welcome message
- `STATE:pot:bet:round:cards:hand:chips:playerbet:players` - Game state
- `ACTION:player:action:amount` - Player action notification
- `WINNER:name:amount:hand:cards` - Winner announcement
- `PLAYER_JOINED:name:count` - Player joined
- `PLAYER_LEFT:name:count` - Player left
- `ERROR:message` - Error message

**Client → Server Messages:**
- `NAME:name` - Send player name
- `ACTION:action:amount` - Perform action
- `CHAT:message` - Send chat message

### Threading Model

**Server (C):**
- Main thread: Accepts new connections
- Game loop thread: Manages game state and timing
- Client threads: One per connected client

**Client (C):**
- Main thread: User input processing
- Receive thread: Message handling from server

### Memory Safety

The C implementation focuses on:
- No dynamic memory allocation in hot paths
- Fixed-size buffers with bounds checking
- Proper mutex locking for shared state
- Thread-safe operations throughout

## Credits

Created as a demonstration of:
- Socket programming in C and Python
- Multi-threaded server architecture
- Game logic implementation
- Hand evaluation algorithms
- Client-server protocol design

Enjoy the game! 🃏♠️♥️♦️♣️
