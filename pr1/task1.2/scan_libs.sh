#!/bin/bash

echo "Search for libs with sin, cos, exp functions"

find /usr/lib -name "*.so*" 2>/dev/null | while read file; do

    output=$(nm -D "$file" 2>/dev/null | grep -E " (sin|cos|exp)$")
    if [ -n "$output" ]; then
        echo "find in: $file"
        echo "$output"
    fi
done
