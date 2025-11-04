# Bug Fix History

## Card Display Bug (Solved)

### Symptoms
- Cards showing as `[?C] [?C]` in client display
- Game immediately advancing through all betting rounds without player actions
- Both players seeing identical placeholder cards (2♣ 2♣)

### Root Causes
1. **Critical Logic Bug** in `is_betting_round_complete()` (server.c line 487):
   - Code had: `return active_count <= 1 || true;`
   - The `|| true` made function always return true
   - This caused game to skip all betting and immediately advance through preflop → flop → turn → river
   - Players never got a chance to act, game auto-advanced before cards were dealt

2. **Placeholder Card Initialization**:
   - Players initialized with 2♣ 2♣ as placeholder in `init_game()` and `add_player()`
   - When game auto-advanced due to bug #1, these placeholder cards were never replaced
   - `send_game_state()` would send placeholder cards before `start_new_hand()` dealt real cards

### Fix Applied
Changed line 487 in server.c from:
```c
return active_count <= 1 || true;
```
to:
```c
return active_count <= 1;
```

### Result
- Game now correctly waits for player actions before advancing rounds
- Cards are properly dealt from deck at start of each hand
- Each player receives unique cards (e.g., Player1: 5♥ A♥, Player2: 9♠ 9♥)
- Game stays in preflop until betting round completes
- No more `[?C] [?C]` display

### Debug Output Added
- Server: Prints `Sending to <name>: STATE:...` for each game state message
- Client: Prints `DEBUG STATE:`, `DEBUG: Parsed X fields`, `DEBUG: hand_str=...`, `DEBUG: card1=... card2=...`
- These can be removed once testing is complete

### Testing
Verified with 2 concurrent clients:
- Cards dealt correctly and uniquely to each player
- Game progression works (preflop → flop → turn → river)
- Betting rounds wait for player input
- No crashes or memory issues observed
