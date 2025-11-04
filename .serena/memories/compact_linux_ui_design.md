# Compact Linux-Style UI (November 2025)

## Current Design Philosophy
User requested UI to be "more compact", "not use separators", and "more like a standard linux tool" (ps, top, netstat, etc.)

## Layout (10 lines total)
```
Pot: 150 | Bet: 50 | Round: turn
Board: [AS] [KH] [QD] [JC] [--]
Hand:  [TH] [9H] | Chips: 950 | Bet: 50

Players:
*D> Alice           950 (50)
    Bob             900 (50)
    Charlie         800 FOLD

** YOUR TURN **
Actions: fold | check | raise <amt> (min 100)
> 
```

## Key Features

### Single-Character Status Indicators
- `*` = You (like current directory marker)
- `D` = Dealer button
- `>` = Current turn (like shell prompt)
- Combined: `*D>` means you're dealer and it's your turn

### Player Format
```
[*][D][>] Name            Chips (Bet) [Status]
```

### No Decorative Elements
- NO separators (======, ------)
- NO centered headers
- NO boxes or borders
- NO extra whitespace
- NO verbose labels

### Information Density
- Single line for pot/bet/round
- Single line for board cards
- Single line for your hand + chips
- One line per player (compact)
- Only show turn prompt when it's your turn

### Card Notation
- Active cards: `[AS]` `[KH]` (2-char rank+suit)
- Not dealt: `[--]` (like N/A in Unix tools)

## Help Menu (Compact)
```
Commands:
  fold              Fold hand
  check             Check (no bet required)
  call              Match current bet
  raise <amount>    Raise bet to <amount>
  chat <message>    Send message to all players
  help              Show this help
  quit              Leave game

Player status:
  *   You
  D   Dealer
  >   Current turn
```

## Implementation
- `client.c` - `display_game_state()` (lines 63-128) - ~65 lines
- `client.c` - `display_help()` (lines 436-450) - ~15 lines

## Design Inspiration
Modeled after standard Unix tools:
- `ps aux` - Compact process list with status flags
- `top` - Single-line header with metrics
- `netstat` - Inline connection data
- `git status` - Status indicators
- `htop` - Information density

## Benefits
- 60% less screen space than previous UI
- Faster to scan (no decorative noise)
- Unix-like familiarity
- Works on tiny terminals
- More game history visible

## Previous Versions
1. Unicode box-drawing (rejected - not pure ASCII)
2. Pure ASCII with separators (rejected - too verbose)
3. **Current: Compact Unix-style** (approved)
