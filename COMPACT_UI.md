# Compact Linux-Style UI

## Design Philosophy

The interface now follows **standard Unix tool conventions** - compact, information-dense, no decorative elements.

Inspired by tools like: `ps`, `top`, `netstat`, `htop`, `git status`

## Key Principles

1. **Information density** - Every line provides value
2. **No separators** - Whitespace implies structure
3. **Single-character indicators** - Like `ps` flags
4. **Inline data** - Related info stays together
5. **Concise prompts** - Only what you need

## Interface Layout

### Full Game State (10 lines)
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

### Status Indicators (Like `ps` Flags)

| Char | Meaning |
|------|---------|
| `*`  | You (like `*` in current directory listings) |
| `D`  | Dealer button |
| `>`  | Current turn (like `>` prompt) |

### Player Line Format
```
[*][D][>] Name            Chips (Bet) [Status]
```

Examples:
- `*D> Alice           950 (50)` - You, dealer, your turn
- `    Bob             900 (50)` - Other player, active
- `    Charlie         800 FOLD` - Other player, folded

### Card Display
- Active cards: `[AS] [KH]` (rank + suit)
- Not yet dealt: `[--]` (like "N/A" in Unix tools)

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
> 
```

## Comparison with Previous Design

### Before (Decorated Style - 25+ lines)
```
======================================================================
                      TEXAS HOLD'EM POKER                            
======================================================================
 POT: 150        chips | CURRENT BET: 50       | ROUND: turn          
======================================================================

----------------------------------------------------------------------
                        COMMUNITY CARDS                              
----------------------------------------------------------------------
  [AS] [KH] [QD] [JC] [??]
----------------------------------------------------------------------

----------------------------------------------------------------------
                            PLAYERS                                  
----------------------------------------------------------------------
>>> Alice           [DEALER]  [*TURN*]  Chips: 950     Bet: 50      <YOU>
    Bob                                 Chips: 900     Bet: 50     
    Charlie                             Chips: 800                  [FOLDED]
----------------------------------------------------------------------

----------------------------------------------------------------------
                          YOUR HAND                                  
----------------------------------------------------------------------
                         [TH] [9H]                                   
----------------------------------------------------------------------
  Your Chips: 950       |  Your Bet This Round: 50                  
----------------------------------------------------------------------

======================================================================
                         *** YOUR TURN! ***                          
======================================================================
```

### After (Compact Style - 10 lines)
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

## Benefits

✅ **60% less screen space** - See more game history  
✅ **Faster scanning** - No decorative noise  
✅ **Unix-like familiarity** - Feels like standard tools  
✅ **Information dense** - More data in less space  
✅ **Professional** - Clean, focused, purposeful  
✅ **Terminal friendly** - Works on tiny screens  
✅ **Accessibility** - Screen readers prefer concise output  

## Unix Tool Analogies

| Tool | Similar Feature |
|------|----------------|
| `ps aux` | Compact player list with status flags |
| `top` | Single-line header with key metrics |
| `netstat` | Inline connection data |
| `git status` | Status indicators (`*`, `D`, `>`) |
| `ls -l` | Columnar data with flags |

## Implementation

- **Lines of code**: Reduced from ~120 to ~65
- **Printf calls**: Reduced from ~50 to ~20
- **Complexity**: Simpler logic, easier to maintain
- **Performance**: Faster rendering (fewer syscalls)

## Testing

Run the game:
```bash
./poker_server &
./poker_client
```

Or see the demo:
```bash
./demo_compact_ui.sh
```

## Result

A poker interface that follows the **Unix philosophy**:
- Do one thing well
- Text-based and composable
- Minimal and efficient
- Information-dense
- No unnecessary decoration

**Like grep, sed, awk - this is poker the Unix way.**
