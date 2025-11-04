# ASCII Poker - Project Overview

## Purpose
ASCII-only terminal-based no-limit Texas Hold'em poker game with client-server architecture.

## Tech Stack
- **Language**: C11
- **Compiler**: gcc with -Wall -Wextra -O2 -pthread -std=c11
- **Threading**: POSIX threads (pthread)
- **Networking**: TCP sockets
- **Build System**: GNU Make

## Architecture
Multi-threaded client-server poker game:

### Server (server.c - 789 lines)
- Main thread: Accepts incoming client connections
- Game loop thread: Manages game state and betting rounds
- Per-client threads: Handle individual player communication
- Thread synchronization: mutex locks for shared game state
- Supports up to 10 concurrent players

### Client (client.c - 532 lines)
- Main thread: User input and command processing
- Receive thread: Handles incoming server messages
- ASCII UI: Terminal-based display of game state
- Thread synchronization: mutex for game state updates

### Poker Logic (poker.c - 448 lines, poker.h - 100 lines)
- Complete Texas Hold'em hand evaluation
- Deck management with shuffling
- Card representation and parsing
- Hand comparison with tiebreakers
- Supports all poker hands from Royal Flush to High Card

## Protocol
Text-based TCP protocol with colon-separated fields:
- STATE messages: `STATE:pot:bet:round:community:hand:chips:bet:players`
- Player data: pipe-separated with nested colons
- Format: `name:chips:bet:folded:all_in:is_dealer:is_current|...`

## Known Issues
Currently debugging card display showing as `[?C] [?C]` - protocol parsing issue being investigated.
