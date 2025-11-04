#!/bin/bash
# Proper interactive test for poker

cd /home/trader/aa3/ascii-poker

# Start first client in background with a named pipe
mkfifo /tmp/poker_client1_input 2>/dev/null || true
mkfifo /tmp/poker_client2_input 2>/dev/null || true

# Client 1
{
    echo "Player1"
    echo ""
    echo ""
    sleep 2
    echo "call"
    sleep 5
    echo "quit"
} > /tmp/poker_client1_input &

./poker_client < /tmp/poker_client1_input 2>&1 | tee /tmp/client1.log &
CLIENT1_PID=$!

sleep 1

# Client 2
{
    echo "Player2"
    echo ""
    echo ""
    sleep 2
    echo "call"
    sleep 5
    echo "quit"
} > /tmp/poker_client2_input &

./poker_client < /tmp/poker_client2_input 2>&1 | tee /tmp/client2.log &
CLIENT2_PID=$!

# Wait for clients
wait $CLIENT1_PID
wait $CLIENT2_PID

echo ""
echo "=== CLIENT 1 LOG ==="
cat /tmp/client1.log | grep -A 20 "DEBUG"

echo ""
echo "=== CLIENT 2 LOG ==="
cat /tmp/client2.log | grep -A 20 "DEBUG"

# Cleanup
rm -f /tmp/poker_client1_input /tmp/poker_client2_input
