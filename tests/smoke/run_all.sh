#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PASS=0
FAIL=0

for test_script in "$SCRIPT_DIR"/test_sec*.sh; do
    echo "--- Running: $(basename "$test_script") ---"
    if bash "$test_script"; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
    fi
    echo ""
done

echo "=== Overall: $PASS suites passed, $FAIL suites failed ==="
[ "$FAIL" -eq 0 ]
