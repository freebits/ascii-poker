# Codebase Structure

## Root Directory Files
- **poker.h** (100 lines) - Header with all type definitions, enums, and function prototypes
- **poker.c** (448 lines) - Core poker game logic (hand evaluation, deck management)
- **server.c** (789 lines) - Multi-threaded game server
- **client.c** (532 lines) - Terminal client with ASCII UI
- **Makefile** (59 lines) - Build system

## Documentation
- **README.md** - Main documentation for both Python and C versions
- **QUICKSTART.md** - Quick start guide for C version
- **COMPARISON.md** - Comparison between C and Python implementations
- **PROJECT_STRUCTURE.md** - Detailed project structure

## Build Artifacts (gitignored)
- **poker_server** (27KB) - Server executable
- **poker_client** (31KB) - Client executable
- **\*.o** - Object files (server.o, client.o, poker.o)
- **\*_debug.log**, **\*_output.log** - Debug logs

## Test Scripts
- **test_debug.sh** - Automated test script with debug output capture

## Key Data Structures

### poker.h
```c
typedef enum { CLUBS, DIAMONDS, HEARTS, SPADES } Suit;
typedef enum { TWO, THREE, ..., KING, ACE } Rank;
typedef enum { HIGH_CARD, ONE_PAIR, ..., ROYAL_FLUSH } HandRank;

typedef struct { Rank rank; Suit suit; } Card;
typedef struct { Card cards[52]; int cards_remaining; } Deck;
typedef struct { HandRank rank; int tiebreakers[5]; int num_tiebreakers; } HandValue;
```

### server.c
```c
typedef struct {
    int socket; char name[64]; int chips;
    Card hand[2]; int bet; int total_bet;
    bool folded; bool all_in; bool active;
    pthread_mutex_t lock;
} Player;

typedef struct {
    Player players[MAX_PLAYERS]; int num_players;
    Deck deck; Card community_cards[5]; int num_community_cards;
    int pot; int current_bet;
    int dealer_index; int current_player_index;
    GameRound round; bool game_active;
    pthread_mutex_t lock;
} GameState;
```

### client.c
```c
typedef struct {
    char name[64]; int chips; int bet;
    bool folded; bool all_in;
    bool is_dealer; bool is_current;
} PlayerInfo;

typedef struct {
    int pot; int current_bet; char round[16];
    char community_cards[128]; Card hand[2];
    int chips; int bet;
    PlayerInfo players[MAX_PLAYERS]; int num_players;
    bool my_turn;
} GameState;
```

## Module Dependencies
- **poker.c** ← Independent, used by both server and client
- **server.c** ← Depends on poker.h/poker.c
- **client.c** ← Depends on poker.h/poker.c
- **Makefile** ← Orchestrates compilation
