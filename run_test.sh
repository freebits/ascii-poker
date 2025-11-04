#!/bin/bash
# Simple debug test for poker client

# Start first client with automated input
{
    sleep 0.5
    echo "Player1"
    sleep 3
    echo "call"
    sleep 2
    echo "quit"
} | ./poker_client 2>&1 &

sleep 2

# Start second client with automated input
{
    sleep 0.5
    echo "Player2"
    sleep 3
    echo "call"
    sleep 2
    echo "quit"
} | ./poker_client 2>&1 &

# Wait for clients to finish
wait
