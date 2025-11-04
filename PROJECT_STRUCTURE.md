# ASCII Poker - Project Structure

```
ascii-poker/
│
├── C Implementation
│   ├── poker.h              - Header file with data structures & function declarations
│   ├── poker.c              - Core poker logic (cards, deck, hand evaluation)
│   ├── server.c             - Multi-threaded game server with pthreads
│   ├── client.c             - Terminal client with ASCII UI
│   ├── Makefile             - Build system for compilation
│   ├── poker_server         - Compiled server binary (after make)
│   └── poker_client         - Compiled client binary (after make)
│
├── Python Implementation
│   ├── poker.py             - Core poker logic (cards, deck, hand evaluation)
│   ├── server.py            - Game server with threading
│   └── client.py            - Terminal client with ASCII UI
│
└── Documentation
    ├── README.md            - Comprehensive documentation for both versions
    ├── QUICKSTART.md        - Quick start guide for C version
    ├── COMPARISON.md        - Comparison between C and Python implementations
    └── PROJECT_STRUCTURE.md - This file

```

## File Purposes

### Core Poker Logic
**poker.h / poker.py**
- Card and Deck structures/classes
- Hand evaluation algorithms
- Card notation and parsing
- Rank/Suit enumerations

**poker.c** (C only)
- Implementation of poker.h
- Hand comparison logic
- Shuffle and deal functions

### Server Components
**server.c / server.py**
- TCP socket server
- Multi-client handling (threading)
- Game state management
- Betting round logic
- Winner determination
- Player management

### Client Components
**client.c / client.py**
- TCP socket client
- ASCII UI rendering
- Command input processing
- Message parsing from server
- Game state display

### Build System
**Makefile** (C only)
- Compilation targets
- Dependency management
- Clean/install targets
- Build flags and options

### Documentation
**README.md**
- Feature overview
- Installation instructions
- Usage guide for both versions
- Game rules and commands
- Network configuration
- Troubleshooting

**QUICKSTART.md**
- Fast start guide for C version
- Basic compilation & running
- Network setup
- Common issues

**COMPARISON.md**
- C vs Python comparison
- Performance notes
- Use case recommendations
- Code metrics

## Compilation Outputs (C)

After running `make`:
- `poker_server` - Server executable (~27 KB)
- `poker_client` - Client executable (~31 KB)
- `*.o` files - Object files (intermediate)

Clean with: `make clean`

## Communication Protocol

Both C and Python implementations use a compatible text-based protocol:
- Line-delimited messages
- Colon-separated fields
- JSON-like structure (Python uses actual JSON)
- Works across language boundaries

## Thread Architecture

### Server (Both Versions)
- **Main thread**: Accepts connections
- **Game loop thread**: Manages game timing
- **Client threads**: One per connected player

### Client (Both Versions)
- **Main thread**: User input
- **Receive thread**: Server message handling

## State Management

### Server State
- Player list with chips, bets, hands
- Deck state
- Community cards
- Pot and current bet
- Dealer and current player indices
- Round (preflop/flop/turn/river)

### Client State
- Current game state snapshot
- Player information
- Own hand
- Community cards
- Turn indicator

## Security Notes

This is a **demonstration project** and includes:
- No authentication
- No encryption
- No input sanitization (basic only)
- No protection against cheating

For production use, add:
- TLS/SSL encryption
- User authentication
- Input validation
- Anti-cheat measures
- Rate limiting
- Logging and monitoring

## Testing

No automated tests included. Manual testing:
1. Start server
2. Connect 2+ clients
3. Play through hands
4. Test all actions (fold, call, raise, check)
5. Test disconnection handling
6. Verify pot calculations
7. Verify hand evaluation

## Future Enhancements

Potential additions:
- Tournament mode
- Multiple tables
- Hand history logging
- Replay functionality
- Statistics tracking
- Configurable blind structure
- Ante support
- Better ASCII card graphics
- ncurses-based UI
- Database integration
- Web interface
- Bot players (AI)

## License

Free to use and modify for educational purposes.

## Contributing

Feel free to fork and improve:
- Add features from enhancement list
- Fix bugs
- Improve documentation
- Optimize performance
- Add tests
- Improve UI

---

Both implementations are fully functional and interoperable. Choose based on your needs and preferences!
