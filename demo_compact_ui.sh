#!/bin/bash

echo "Compact Linux-Style Poker UI Demo"
echo ""
echo "Example game state:"
echo ""
cat << 'EOF'
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
EOF

echo ""
echo "Features:"
echo "  • Information-dense like standard Unix tools (ps, top, netstat)"
echo "  • No decorative separators or whitespace"
echo "  • Single-character status indicators (* = you, D = dealer, > = turn)"
echo "  • Compact player list with inline stats"
echo "  • Clear, concise action prompts"
echo "  • Everything visible at once - minimal scrolling"
echo ""
echo "Help output:"
echo ""
cat << 'EOF'

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
EOF

echo ""
echo "Comparison:"
echo "  Before: ~25 lines with separators and headers"
echo "  After:  ~10 lines of pure information"
echo ""
echo "Just like ps, top, netstat - maximum info, minimum space!"
