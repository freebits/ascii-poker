#!/bin/bash
# Demo the new UI

cd /home/trader/aa3/ascii-poker

echo "Creating a mock game state display..."
echo ""

cat > test_ui_demo.c << 'EOF'
#include "poker.h"
#include <stdio.h>
#include <string.h>

void demo_ui() {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║                         TEXAS HOLD'EM POKER                          ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════╣\n");
    printf("║ POT: 150        chips │ CURRENT BET: 50       │ ROUND: turn          ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────┐\n");
    printf("│                          COMMUNITY CARDS                            │\n");
    printf("├─────────────────────────────────────────────────────────────────────┤\n");
    printf("│  ┌────┐ ┌────┐ ┌────┐ ┌────┐                                        │\n");
    printf("│  │ AS │ │ KH │ │ QD │ │ JC │                                        │\n");
    printf("│  └────┘ └────┘ └────┘ └────┘                                        │\n");
    printf("└─────────────────────────────────────────────────────────────────────┘\n\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────┐\n");
    printf("│                              PLAYERS                                │\n");
    printf("├─────────────────────────────────────────────────────────────────────┤\n");
    printf("│ ► Alice          [D]     Chips: 950    Bet: 50      (YOU)     │\n");
    printf("│   Bob                ⏰  Chips: 900    Bet: 25                 │\n");
    printf("│   Charlie        [D]     Chips: 800               [FOLDED]  │\n");
    printf("└─────────────────────────────────────────────────────────────────────┘\n\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────┐\n");
    printf("│                            YOUR HAND                                │\n");
    printf("├─────────────────────────────────────────────────────────────────────┤\n");
    printf("│          ┌────┐ ┌────┐                                              │\n");
    printf("│          │ TH │ │ 9H │                                              │\n");
    printf("│          └────┘ └────┘                                              │\n");
    printf("├─────────────────────────────────────────────────────────────────────┤\n");
    printf("│  Your Chips: 950      │  Your Bet This Round: 50                   │\n");
    printf("└─────────────────────────────────────────────────────────────────────┘\n");
    
    printf("\n⏳ Waiting for other players...\n");
    printf("\nCommands: fold | check | call | raise <amount> | chat <message> | help | quit\n");
    printf("> \n");
}

int main() {
    printf("\n\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("                  NEW IMPROVED UI DEMONSTRATION\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    demo_ui();
    
    printf("\n\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("                         KEY IMPROVEMENTS\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    printf("✅ Box-drawing characters for professional table layout\n");
    printf("✅ Community cards displayed as visual cards\n");
    printf("✅ Clear player status indicators:\n");
    printf("   • ► marker for your position\n");
    printf("   • [D] for dealer\n");
    printf("   • ⏰ for current player's turn\n");
    printf("✅ Separated sections with clear headers\n");
    printf("✅ Your hand displayed prominently with card graphics\n");
    printf("✅ Action menu shows exact chip amounts needed\n");
    printf("✅ Better spacing and organization\n\n");
    
    return 0;
}
EOF

gcc -o test_ui_demo test_ui_demo.c -std=c11
./test_ui_demo
rm -f test_ui_demo test_ui_demo.c

echo ""
