#!/bin/bash
# Test script to capture debug output from poker game

echo "Starting poker server in background..."
./poker_server > server_output.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

sleep 2

echo "Starting first client..."
(echo "Player1"; sleep 2; echo "call"; sleep 1; echo "quit") | ./poker_client 2> client1_debug.log > client1_output.log &
CLIENT1_PID=$!

sleep 1

echo "Starting second client..."
(echo "Player2"; sleep 2; echo "call"; sleep 1; echo "quit") | ./poker_client 2> client2_debug.log > client2_output.log &
CLIENT2_PID=$!

sleep 5

echo "Waiting for processes to complete..."
wait $CLIENT1_PID 2>/dev/null
wait $CLIENT2_PID 2>/dev/null

echo "Stopping server..."
kill $SERVER_PID 2>/dev/null

echo ""
echo "=== SERVER OUTPUT ==="
cat server_output.log

echo ""
echo "=== CLIENT 1 DEBUG (stderr) ==="
cat client1_debug.log

echo ""
echo "=== CLIENT 2 DEBUG (stderr) ==="
cat client2_debug.log

echo ""
echo "=== Analysis ==="
echo "Looking for STATE messages in server output..."
grep "Sending to" server_output.log | head -3

echo ""
echo "Looking for DEBUG STATE in client1..."
grep "DEBUG" client1_debug.log | head -5

echo ""
echo "Done! Check the log files for full details."
