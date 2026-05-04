#!/bin/bash
# bench/run_bench.sh - Performance measurement harness for myscp
#
# Generates payloads of varying sizes, runs myscp-send against the target
# host (default 127.0.0.1), captures wall time and peak RSS via
# /usr/bin/time, computes throughput in MB/s, and writes a markdown
# table to bench/results.md.
#
# Usage:
#   ./bench/run_bench.sh                          # default sizes: 100M 1G
#   SIZES="100M 500M 1G 5G" ./bench/run_bench.sh
#   HOST=192.168.1.50 ./bench/run_bench.sh        # transfer to a remote host

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BENCH_DIR="$PROJECT_ROOT/bench"
SANDBOX="$BENCH_DIR/sandbox"
RESULTS="$BENCH_DIR/results.md"

HOST="${HOST:-127.0.0.1}"
SIZES="${SIZES:-100M 1G}"

UNAME_S="$(uname -s)"
case "$UNAME_S" in
    Darwin)  TIME_FLAG="-l" ;;
    Linux)   TIME_FLAG="-v" ;;
    *)       echo "❌ Unsupported OS: $UNAME_S"; exit 1 ;;
esac

if ! command -v myscp-send >/dev/null 2>&1; then
    echo "❌ myscp-send not found on PATH. Run 'make install' first." >&2
    exit 1
fi

if ! command -v /usr/bin/time >/dev/null 2>&1; then
    echo "❌ /usr/bin/time not found (needed for resource measurement)." >&2
    exit 1
fi

mkdir -p "$SANDBOX"

# Convert a human-readable size (e.g. "100M", "1G") to bytes (binary units).
size_to_bytes() {
    local s="$1"
    local n="${s%[KMGkmg]}"
    case "$s" in
        *K|*k) echo "$((n * 1024))" ;;
        *M|*m) echo "$((n * 1024 * 1024))" ;;
        *G|*g) echo "$((n * 1024 * 1024 * 1024))" ;;
        *)     echo "$n" ;;
    esac
}

# Parse wall time (seconds) and peak RSS (MB) from /usr/bin/time output.
# macOS -l and Linux -v differ in format, so we handle each separately.
parse_time_output() {
    local logfile="$1"
    if [ "$UNAME_S" = "Darwin" ]; then
        WALL=$(awk '/real/ { print $1; exit }' "$logfile")
        RSS_BYTES=$(awk '/maximum resident set size/ { print $1; exit }' "$logfile")
        RSS_MB=$(awk -v b="$RSS_BYTES" 'BEGIN { printf "%.1f", b / 1024 / 1024 }')
    else
        WALL=$(awk -F': ' '/Elapsed \(wall clock\) time/ { print $NF }' "$logfile" \
            | awk -F: '{ if (NF==3) printf "%.2f", $1*3600 + $2*60 + $3; else printf "%.2f", $1*60 + $2 }')
        RSS_KB=$(awk -F': ' '/Maximum resident set size/ { print $NF }' "$logfile")
        RSS_MB=$(awk -v k="$RSS_KB" 'BEGIN { printf "%.1f", k / 1024 }')
    fi
}

{
    echo "# MySCP Benchmark Results"
    echo
    echo "- Host: \`$HOST\`"
    echo "- OS: \`$UNAME_S\`"
    echo "- Date: \`$(date)\`"
    echo
    echo "| Size | Wall Time (s) | Throughput (MB/s) | Peak RSS (MB) |"
    echo "| ---: | ---: | ---: | ---: |"
} > "$RESULTS"

for SIZE in $SIZES; do
    PAYLOAD="$SANDBOX/payload_${SIZE}.bin"
    TIME_LOG="$SANDBOX/time_${SIZE}.log"
    DEST_DIR="$SANDBOX/dest_${SIZE}/"
    mkdir -p "$DEST_DIR"

    BYTES=$(size_to_bytes "$SIZE")
    BLOCKS=$((BYTES / 1048576))

    echo "[BENCH] Generating ${SIZE} payload..."
    if [ "$UNAME_S" = "Darwin" ]; then
        dd if=/dev/urandom of="$PAYLOAD" bs=1m count="$BLOCKS" 2>/dev/null
    else
        dd if=/dev/urandom of="$PAYLOAD" bs=1M count="$BLOCKS" 2>/dev/null
    fi

    echo "[BENCH] Transferring ${SIZE} via myscp-send..."
    /usr/bin/time $TIME_FLAG -o "$TIME_LOG" \
        myscp-send "$HOST" "$PAYLOAD" "$DEST_DIR" >/dev/null

    parse_time_output "$TIME_LOG"

    THROUGHPUT=$(awk -v b="$BYTES" -v t="$WALL" \
        'BEGIN { if (t > 0) printf "%.1f", (b / 1024 / 1024) / t; else print "n/a" }')

    printf "| %s | %s | %s | %s |\n" "$SIZE" "$WALL" "$THROUGHPUT" "$RSS_MB" >> "$RESULTS"
    echo "[BENCH] ${SIZE}: ${WALL}s, ${THROUGHPUT} MB/s, peak RSS ${RSS_MB} MB"
done

echo
echo "✅ Results written to $RESULTS"
