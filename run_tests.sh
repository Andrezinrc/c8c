#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "=== RUNNING TEST SUITE ==="
start_total=$(date +%s%N)

for test_file in tests/*.c8; do
    test=$(basename "$test_file" .c8)
    
    ./c8asm "tests/$test.c8" "tests/$test.ch8" > /dev/null 2>&1
    hexdump -C "tests/$test.ch8" > "tests/current_$test.txt"

    if diff -q "tests/current_$test.txt" "tests/expected/$test.txt" > /dev/null 2>&1; then
        echo -e "[${GREEN} PASS ${NC}] $test"
    else
        echo -e "[${RED} FAIL ${NC}] $test"
    fi

    rm -f "tests/current_$test.txt"
    rm -f "tests/$test.ch8"
done
