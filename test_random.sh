#!/bin/bash
# Test to verify deck shuffling produces different results

cd /home/trader/aa3/ascii-poker

echo "Testing deck shuffle randomness..."
echo "Each shuffle should produce different card order."
echo ""

# Create a simple test program
cat > test_shuffle.c << 'EOF'
#include "poker.h"
#include <stdio.h>

int main() {
    Deck deck;
    
    for (int test = 0; test < 5; test++) {
        deck_init(&deck);
        deck_shuffle(&deck);
        
        printf("Shuffle %d: ", test + 1);
        for (int i = 0; i < 10; i++) {
            char card_str[4];
            card_to_string(deck.cards[i], card_str);
            printf("%s ", card_str);
        }
        printf("...\n");
    }
    
    return 0;
}
EOF

# Compile test
gcc -o test_shuffle test_shuffle.c poker.o -std=c11

# Run test
./test_shuffle

# Cleanup
rm -f test_shuffle test_shuffle.c

echo ""
echo "✓ If the 5 shuffles show different card orders, randomness is working!"
