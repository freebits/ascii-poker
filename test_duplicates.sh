#!/bin/bash
# Test to verify no duplicate cards are dealt in a hand

cd /home/trader/aa3/ascii-poker

echo "Testing for duplicate cards in a poker hand..."
echo ""

cat > test_duplicates.c << 'EOF'
#include "poker.h"
#include <stdio.h>
#include <stdbool.h>

bool is_duplicate(Card cards[], int count, Card new_card) {
    for (int i = 0; i < count; i++) {
        if (cards[i].rank == new_card.rank && cards[i].suit == new_card.suit) {
            return true;
        }
    }
    return false;
}

int main() {
    Deck deck;
    Card all_dealt[52];
    int dealt_count = 0;
    bool duplicates_found = false;
    
    printf("Simulating a full poker hand with 2 players:\n\n");
    
    for (int test = 0; test < 3; test++) {
        printf("Test %d:\n", test + 1);
        deck_init(&deck);
        deck_shuffle(&deck);
        dealt_count = 0;
        duplicates_found = false;
        
        // Deal hole cards for 2 players
        Card p1_hand[2], p2_hand[2];
        p1_hand[0] = deck_deal(&deck);
        p1_hand[1] = deck_deal(&deck);
        p2_hand[0] = deck_deal(&deck);
        p2_hand[1] = deck_deal(&deck);
        
        all_dealt[dealt_count++] = p1_hand[0];
        all_dealt[dealt_count++] = p1_hand[1];
        all_dealt[dealt_count++] = p2_hand[0];
        all_dealt[dealt_count++] = p2_hand[1];
        
        char c1[4], c2[4], c3[4], c4[4];
        card_to_string(p1_hand[0], c1);
        card_to_string(p1_hand[1], c2);
        card_to_string(p2_hand[0], c3);
        card_to_string(p2_hand[1], c4);
        printf("  Player 1: %s %s\n", c1, c2);
        printf("  Player 2: %s %s\n", c3, c4);
        
        // Deal flop (3 cards)
        Card flop[3];
        for (int i = 0; i < 3; i++) {
            flop[i] = deck_deal(&deck);
            if (is_duplicate(all_dealt, dealt_count, flop[i])) {
                printf("  ❌ DUPLICATE FOUND in flop!\n");
                duplicates_found = true;
            }
            all_dealt[dealt_count++] = flop[i];
        }
        
        char f1[4], f2[4], f3[4];
        card_to_string(flop[0], f1);
        card_to_string(flop[1], f2);
        card_to_string(flop[2], f3);
        printf("  Flop: %s %s %s\n", f1, f2, f3);
        
        // Deal turn
        Card turn = deck_deal(&deck);
        if (is_duplicate(all_dealt, dealt_count, turn)) {
            printf("  ❌ DUPLICATE FOUND in turn!\n");
            duplicates_found = true;
        }
        all_dealt[dealt_count++] = turn;
        
        char t[4];
        card_to_string(turn, t);
        printf("  Turn: %s\n", t);
        
        // Deal river
        Card river = deck_deal(&deck);
        if (is_duplicate(all_dealt, dealt_count, river)) {
            printf("  ❌ DUPLICATE FOUND in river!\n");
            duplicates_found = true;
        }
        all_dealt[dealt_count++] = river;
        
        char r[4];
        card_to_string(river, r);
        printf("  River: %s\n", r);
        
        printf("  Cards remaining in deck: %d\n", deck.cards_remaining);
        printf("  Total cards dealt: %d\n", dealt_count);
        
        if (!duplicates_found) {
            printf("  ✅ No duplicates - all cards unique!\n");
        }
        printf("\n");
    }
    
    return 0;
}
EOF

# Compile test
gcc -o test_duplicates test_duplicates.c poker.o -std=c11

# Run test
./test_duplicates

# Cleanup
rm -f test_duplicates test_duplicates.c

echo "✅ If all tests show 'No duplicates', the deck management is working correctly!"
