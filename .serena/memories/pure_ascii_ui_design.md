# Pure ASCII UI Design (November 2025)

## Design Philosophy
User explicitly requested **pure ASCII only** - no Unicode box-drawing characters, no emojis, no special symbols.

## Character Set
- **Equals (=)**: Major headers and emphasis areas
- **Hyphens (-)**: Section dividers and separators  
- **Pipes (|)**: Inline field separators
- **Brackets []**: Cards and status labels
- **Angle brackets <>**: Player markers
- **Greater-than >>>**: "You are here" indicator

## Cards Format
- Dealt cards: `[AS] [KH] [QD]` (simple brackets)
- Face-down: `[??]`
- No fancy box-drawing, just brackets

## Status Indicators
- `>>>` - Your position in player list (highly visible)
- `[DEALER]` - Dealer button (descriptive label)
- `[*TURN*]` - Current player's turn (action indicator)
- `<YOU>` - Additional player marker (confirmation)
- `[FOLDED]` - Player folded
- `[ALL-IN]` - Player all-in

## Layout Structure

### Headers
```
======================================================================
                      TEXAS HOLD'EM POKER                            
======================================================================
 POT: 150        chips | CURRENT BET: 50       | ROUND: turn          
======================================================================
```

### Sections
```
----------------------------------------------------------------------
                        COMMUNITY CARDS                              
----------------------------------------------------------------------
  [AS] [KH] [QD] [JC] [??]
----------------------------------------------------------------------
```

### Player List
```
>>> Alice           [DEALER]  [*TURN*]  Chips: 950     Bet: 50      <YOU>
    Bob                                 Chips: 900     Bet: 50     
    Charlie                             Chips: 800                  [FOLDED]
```

## Key Benefits
1. **Universal compatibility** - Works on ANY terminal
2. **No encoding issues** - Pure ASCII (0x20-0x7E)
3. **SSH-friendly** - Perfect for remote sessions
4. **Screen reader compatible** - Accessible design
5. **Clear hierarchy** - Visual organization through spacing
6. **Easy to scan** - Strong visual anchors (>>>)

## Implementation Files
- `client.c` - `display_game_state()` function (line 63-177)
- `client.c` - `display_help()` function (line 464-492)

## Testing
Run `./demo_ascii_ui.sh` to see the full interface layout.

## Design Constraints
- NO Unicode characters (no ╔ ╗ ║ ═ ┌ ┐ │ ─)
- NO emojis (no 🎲 ⏰ ►)  
- ONLY standard ASCII printable characters
- Focus on clarity through layout and spacing
