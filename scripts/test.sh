#!/bin/bash
echo "this script will count from 1 to 10 and then prints selected message"
for i in {1..10}; do
    echo "=> $i"
    sleep 1
done

echo $1