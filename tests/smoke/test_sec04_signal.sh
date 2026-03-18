#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
SERVER_BIN="$PROJECT_DIR/build/cserve"
PORT=18082
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

# Manual cleanup -- we control shutdown timing
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

echo "=== SEC-04: Signal Handling Tests ==="

# Test 1: Server starts and handles a request
STATUS=$(curl -s -o /dev/null -w "%{http_code}" \
    "http://localhost:$PORT/static/index.html" 2>/dev/null || echo "000")
if [ "$STATUS" = "200" ]; then
    echo "PASS: Server handles request before signal (got $STATUS)"
    PASS=$((PASS + 1))
else
    echo "FAIL: Server handles request before signal (expected 200, got $STATUS)"
    FAIL=$((FAIL + 1))
fi

# Test 2: SIGTERM causes clean shutdown
kill -TERM "$SERVER_PID"
sleep 2

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "PASS: Server exited after SIGTERM"
    PASS=$((PASS + 1))
else
    echo "FAIL: Server still running after SIGTERM"
    kill -9 "$SERVER_PID" 2>/dev/null || true
    FAIL=$((FAIL + 1))
fi

# Test 3: Exit code is 0 (clean shutdown) or 143 (SIGTERM default)
wait "$SERVER_PID" 2>/dev/null
EXIT_CODE=$?
if [ "$EXIT_CODE" -eq 0 ] || [ "$EXIT_CODE" -eq 143 ]; then
    echo "PASS: Clean exit code ($EXIT_CODE)"
    PASS=$((PASS + 1))
else
    echo "FAIL: Non-clean exit code ($EXIT_CODE)"
    FAIL=$((FAIL + 1))
fi

# Prevent cleanup trap from trying to kill already-exited process
SERVER_PID=0

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
