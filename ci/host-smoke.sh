#!/usr/bin/env bash
#
# host-smoke.sh -- end-to-end check of the dedicated server
#                  (MMO_PLAN Phase 6).
#
# tests/test_server.c covers the protocol and the persistence rules over
# the in-memory transport, deterministically. This covers the two things
# that test cannot: real sockets, and the two binaries actually being
# able to talk to each other. It asserts, in one run:
#
#   1. the server starts, listens, and ticks on its own clock;
#   2. a real client connects over TCP and is given an identity and the
#      world;
#   3. the server keeps ticking after that client leaves;
#   4. the checkpoint it writes replays to the same hash in
#      saltmarch_replay -- i.e. the server's world is an ordinary
#      (seed, log) world, not a private format.
#
# Usage: ci/host-smoke.sh <build-dir> [port]

set -uo pipefail

BUILD="${1:?usage: host-smoke.sh <build-dir> [port]}"
PORT="${2:-7788}"
EXE=""
[ -x "$BUILD/saltmarch_host" ]     && EXE=""
[ -x "$BUILD/saltmarch_host.exe" ] && EXE=".exe"

HOST_BIN="$BUILD/saltmarch_host$EXE"
GAME_BIN="$BUILD/saltmarch$EXE"
REPLAY_BIN="$BUILD/saltmarch_replay$EXE"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

WORLD="$WORK/world.smlog"
SRVLOG="$WORK/server.log"
CLILOG="$WORK/client.log"

fail() { printf '  FAIL: %s\n' "$1"; FAILURES=$((FAILURES + 1)); }
pass() { printf '  ok:   %s\n' "$1"; }
FAILURES=0

echo "== host smoke test: $HOST_BIN (port $PORT) =="

for BIN in "$HOST_BIN" "$GAME_BIN" "$REPLAY_BIN"; do
    if [ ! -x "$BIN" ]; then
        echo "  FAIL: missing binary: $BIN"
        exit 1
    fi
done

# 200 ticks = 20 seconds of world time; the run ends on its own, so a
# hung server cannot stall CI and no `timeout` (GNU-only) is needed.
"$HOST_BIN" --world "$WORLD" --seed 4242 --port "$PORT" \
            --ticks 200 --checkpoint-seconds 0 >"$SRVLOG" 2>&1 &
HOST_PID=$!

# Give it a moment to bind before connecting.
sleep 2

if ! kill -0 "$HOST_PID" 2>/dev/null; then
    fail "server exited immediately"
    sed 's/^/  | /' "$SRVLOG"
    exit 1
fi
pass "server started and is listening"

# A real client, headless. --as 1 claims the founding island, which is
# the identity a fresh world leaves unheld on a dedicated server.
SDL_VIDEODRIVER=dummy "$GAME_BIN" --join "127.0.0.1:$PORT" --as 1 \
    >"$CLILOG" 2>&1 &
CLI_PID=$!
sleep 6
kill -TERM "$CLI_PID" 2>/dev/null
wait "$CLI_PID" 2>/dev/null

wait "$HOST_PID" 2>/dev/null
HOST_RC=$?

echo "--- server output ---"
sed 's/^/  | /' "$SRVLOG"
echo "---------------------"

if [ "$HOST_RC" -eq 0 ]; then
    pass "server ran its ticks and exited cleanly"
else
    fail "server exited with rc=$HOST_RC"
fi

if grep -q "client joined as player" "$SRVLOG"; then
    pass "client connected over TCP and was given an identity"
else
    fail "server never logged a join"
    sed 's/^/  | /' "$CLILOG"
fi

if grep -q "world installed at tick" "$CLILOG"; then
    pass "client received and installed the server's world"
else
    fail "client never installed a world"
    sed 's/^/  | /' "$CLILOG"
fi

if grep -q "stopping at tick 200" "$SRVLOG"; then
    pass "server kept ticking through the client's departure"
else
    fail "server did not reach its tick target"
fi

if [ -f "$WORLD" ]; then
    pass "server wrote a checkpoint"
    if "$REPLAY_BIN" --replay "$WORLD" >"$WORK/replay.log" 2>&1; then
        pass "the checkpoint replays to the same hash"
    else
        fail "the checkpoint failed to replay:"
        sed 's/^/  | /' "$WORK/replay.log"
    fi
else
    fail "no checkpoint written"
fi

echo
if [ "$FAILURES" -eq 0 ]; then
    echo "HOST SMOKE TEST PASSED"
    exit 0
fi
echo "HOST SMOKE TEST FAILED ($FAILURES check(s))"
exit 1
