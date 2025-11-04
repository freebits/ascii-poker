# Deck Management - No Duplicate Cards

## How the Deck Works

### Structure
```c
typedef struct {
    Card cards[52];        // All 52 cards
    int cards_remaining;   // Tracks how many cards left to deal
} Deck;
```

### Key Functions

#### 1. `deck_init(Deck *deck)`
- Creates all 52 cards (13 ranks × 4 suits)
- Sets `cards_remaining = 52`
- Cards are in a predictable order initially

#### 2. `deck_shuffle(Deck *deck)`
- Uses Fisher-Yates algorithm with `/dev/urandom`
- Randomizes all 52 cards in the array
- Does NOT change `cards_remaining`

#### 3. `deck_deal(Deck *deck)`
```c
Card deck_deal(Deck *deck) {
    if (deck->cards_remaining <= 0) {
        Card invalid = {TWO, CLUBS};
        return invalid;
    }
    deck->cards_remaining--;
    return deck->cards[deck->cards_remaining];
}
```
- Decrements `cards_remaining` BEFORE returning
- Returns card from position `cards_remaining`
- **Guarantees no duplicates** - each position dealt only once

#### 4. `deck_reset(Deck *deck)`
- Calls `deck_init()` to reset all 52 cards
- Calls `deck_shuffle()` to randomize
- Used at start of each new hand

## Card Dealing Sequence in a Hand

### Example with 2 Players:

1. **Start of hand**: `deck_reset(&game.deck)` → `cards_remaining = 52`

2. **Deal hole cards** (in `start_new_hand()`):
   ```c
   player1.hand[0] = deck_deal(&deck);  // cards_remaining = 51
   player1.hand[1] = deck_deal(&deck);  // cards_remaining = 50
   player2.hand[0] = deck_deal(&deck);  // cards_remaining = 49
   player2.hand[1] = deck_deal(&deck);  // cards_remaining = 48
   ```

3. **Deal flop** (in `next_round()` when round transitions from preflop to flop):
   ```c
   deck_deal_multiple(&deck, &community_cards[0], 3);
   // cards_remaining = 45
   ```

4. **Deal turn** (in `next_round()` when round transitions from flop to turn):
   ```c
   community_cards[3] = deck_deal(&deck);  // cards_remaining = 44
   ```

5. **Deal river** (in `next_round()` when round transitions from turn to river):
   ```c
   community_cards[4] = deck_deal(&deck);  // cards_remaining = 43
   ```

### Final State:
- **9 cards dealt total** (4 hole + 5 community)
- **43 cards remaining** in deck
- **All cards unique** - mathematically impossible to deal duplicates

## Why It's Impossible to Deal Duplicates

1. **Single deck instance** - `game.deck` is shared across entire hand
2. **Monotonic decrement** - `cards_remaining` only decreases, never increases during a hand
3. **No reuse** - Each `deck_deal()` returns a different position in the array
4. **Fresh deck per hand** - `deck_reset()` is called at start of each hand

## Verification
Tested with `test_duplicates.sh`:
- Simulates full poker hands (2 players + 5 community cards)
- Checks every card against all previously dealt cards
- **Result**: ✅ No duplicates found in any test

## Edge Cases Handled
- **Insufficient cards**: If `cards_remaining <= 0`, returns invalid card (TWO of CLUBS)
- **Multiple players**: Works correctly with up to 10 players (20 hole cards + 5 community = 25 cards max)
- **Thread safety**: Protected by `game.lock` mutex in `start_new_hand()` and `next_round()`
