#!/bin/bash

echo "============================================================"
echo "           UI EVOLUTION: From Decorated to Unix-Style"
echo "============================================================"
echo ""
echo "VERSION 1: Unicode Box-Drawing (REJECTED - Not pure ASCII)"
echo "------------------------------------------------------------"
cat << 'EOF'
╔══════════════════════════════════════════════════════════════╗
║                    TEXAS HOLD'EM POKER                       ║
╠══════════════════════════════════════════════════════════════╣
║ POT: 150  │ BET: 50  │ ROUND: turn                          ║
╚══════════════════════════════════════════════════════════════╝
┌──────────────────────────────────────────────────────────────┐
│                      COMMUNITY CARDS                         │
├──────────────────────────────────────────────────────────────┤
│  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐                        │
│  │ AS │ │ KH │ │ QD │ │ JC │ │ ?? │                        │
│  └────┘ └────┘ └────┘ └────┘ └────┘                        │
└──────────────────────────────────────────────────────────────┘
EOF
echo "Lines: 25+  |  Requires: UTF-8  |  Status: ❌ Rejected"
echo ""

echo "VERSION 2: Pure ASCII with Separators (REJECTED - Too verbose)"
echo "------------------------------------------------------------"
cat << 'EOF'
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
EOF
echo "Lines: 25+  |  Requires: ASCII  |  Status: ❌ Too much decoration"
echo ""

echo "VERSION 3: Compact Linux-Style (CURRENT - Approved! ✅)"
echo "------------------------------------------------------------"
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
echo "Lines: 10   |  Requires: ASCII  |  Status: ✅ Perfect!"
echo ""

echo "============================================================"
echo "                        METRICS"
echo "============================================================"
echo ""
printf "%-20s %10s %10s %10s\n" "Metric" "V1 (UTF-8)" "V2 (ASCII)" "V3 (Compact)"
printf "%-20s %10s %10s %10s\n" "--------------------" "----------" "----------" "----------"
printf "%-20s %10s %10s %10s\n" "Screen lines" "~30" "~25" "~10"
printf "%-20s %10s %10s %10s\n" "Separator lines" "14" "14" "0"
printf "%-20s %10s %10s %10s\n" "Code lines" "~210" "~180" "~120"
printf "%-20s %10s %10s %10s\n" "Printf calls" "~60" "~50" "~20"
printf "%-20s %10s %10s %10s\n" "Encoding" "UTF-8" "ASCII" "ASCII"
printf "%-20s %10s %10s %10s\n" "Unix-like" "No" "No" "Yes"
printf "%-20s %10s %10s %10s\n" "Information/line" "Low" "Medium" "High"
echo ""

echo "============================================================"
echo "                   DESIGN PHILOSOPHY"
echo "============================================================"
echo ""
echo "V1: Decorative, visual appeal, modern terminals only"
echo "V2: Universal compatibility, but verbose and decorative"
echo "V3: Unix philosophy - minimal, efficient, information-dense"
echo ""
echo "Inspired by: ps, top, netstat, git status, htop"
echo ""
echo "Final Result: A poker game that feels like a standard"
echo "              Linux system tool - fast, compact, efficient."
echo "============================================================"
