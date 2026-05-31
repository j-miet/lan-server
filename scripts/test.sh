#!/bin/bash
echo "this script will count from 1 to 5, then prints input and selected message"
for i in {1..5}; do
    echo "=> $i"
    sleep 1
done

echo "Input: $1"
echo "Message: $2"