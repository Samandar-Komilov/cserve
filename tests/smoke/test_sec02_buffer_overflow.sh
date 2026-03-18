#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
SERVER_BIN="$PROJECT_DIR/build/cserve"
PORT=18081
PASS=0
FAIL=0

# Build if needed
if [ ! -f "$SERVER_BIN" ]; then
    echo "Building server..."
    mkdir -p "$PROJECT_DIR/build"
    cd "$PROJECT_DIR/build" && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j"$(nproc)"
fi

# Set up temp directory with config and static files
TMPDIR=$(mktemp -d)
mkdir -p "$TMPDIR/static"
echo "test content" > "$TMPDIR/static/index.html"

cat > "$TMPDIR/cserver.ini" <<CONF
port=$PORT
static_dir=$TMPDIR/static
CONF

cleanup() {
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

cd "$TMPDIR" && "$SERVER_BIN" &
SERVER_PID=$!
sleep 1

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "FATAL: Server failed to start"
    exit 1
fi

assert_status() {
    local desc="$1" expected="$2" actual="$3"
    if [ "$actual" = "$expected" ]; then
        echo "PASS: $desc (got $actual)"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $desc (expected $expected, got $actual)"
        FAIL=$((FAIL + 1))
    fi
}

echo "=== SEC-02: Buffer Overflow Tests ==="

# Test 1: Oversized headers (>8KB) -- should get 431 or connection closed
LARGE_HEADER=$(python3 -c "print('X-Large: ' + 'A' * 9000)")
STATUS=$(curl -s -o /dev/null -w "%{http_code}" -H "$LARGE_HEADER" \
    "http://localhost:$PORT/static/index.html" 2>/dev/null || echo "000")
# Accept 431 (proper), 400, or 000 (connection reset -- still safe)
if [ "$STATUS" = "431" ] || [ "$STATUS" = "400" ] || [ "$STATUS" = "000" ]; then
    echo "PASS: Oversized header rejected (got $STATUS)"
    PASS=$((PASS + 1))
else
    echo "FAIL: Oversized header not rejected (got $STATUS)"
    FAIL=$((FAIL + 1))
fi

# Test 2: Server still works after oversized request
sleep 1
STATUS=$(curl -s -o /dev/null -w "%{http_code}" \
    "http://localhost:$PORT/static/index.html" 2>/dev/null || echo "000")
assert_status "Server alive after oversized request" "200" "$STATUS"

# Test 3: Verify server did not crash (process still alive)
if kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "PASS: Server process still alive after overflow attempt"
    PASS=$((PASS + 1))
else
    echo "FAIL: Server process died after overflow attempt"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
