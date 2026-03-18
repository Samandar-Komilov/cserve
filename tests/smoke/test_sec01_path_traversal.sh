#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
SERVER_BIN="$PROJECT_DIR/build/cserve"
PORT=18080
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
echo "secret" > "$TMPDIR/secret.txt"

# Config: key=value format (no INI sections)
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

# Start server in background (CWD matters for BASE_DIR resolution)
cd "$TMPDIR" && "$SERVER_BIN" &
SERVER_PID=$!
sleep 1

# Verify server started
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

echo "=== SEC-01: Path Traversal Tests ==="

# Test 1: Basic /../ traversal -- should be blocked (403 or 404)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is \
    "http://localhost:$PORT/static/../secret.txt" 2>/dev/null || echo "000")
if [ "$STATUS" = "403" ] || [ "$STATUS" = "404" ]; then
    echo "PASS: Basic /../ blocked (got $STATUS)"
    PASS=$((PASS + 1))
else
    echo "FAIL: Basic /../ not blocked (got $STATUS)"
    FAIL=$((FAIL + 1))
fi

# Test 2: Percent-encoded traversal (%2e%2e%2f)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is \
    "http://localhost:$PORT/static/%2e%2e/secret.txt" 2>/dev/null || echo "000")
if [ "$STATUS" = "403" ] || [ "$STATUS" = "404" ]; then
    echo "PASS: %2e%2e encoded traversal blocked (got $STATUS)"
    PASS=$((PASS + 1))
else
    echo "FAIL: %2e%2e encoded traversal not blocked (got $STATUS)"
    FAIL=$((FAIL + 1))
fi

# Test 3: Double-encoded traversal (%252e%252e)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is \
    "http://localhost:$PORT/static/%252e%252e/secret.txt" 2>/dev/null || echo "000")
# After one decode pass, this becomes %2e%2e which realpath won't resolve as ..
# Should be 404 (file not found) or 403
if [ "${STATUS:0:2}" = "40" ]; then
    echo "PASS: Double-encoded traversal blocked (got $STATUS)"
    PASS=$((PASS + 1))
else
    echo "FAIL: Double-encoded traversal not blocked (got $STATUS)"
    FAIL=$((FAIL + 1))
fi

# Test 4: Valid file request still works
STATUS=$(curl -s -o /dev/null -w "%{http_code}" \
    "http://localhost:$PORT/static/index.html" 2>/dev/null || echo "000")
assert_status "Valid file still served" "200" "$STATUS"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
