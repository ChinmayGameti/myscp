#!/bin/bash

# Ensure we are running from the root of the project
if [ ! -d "test" ]; then
    echo "❌ Please run this script from the root project directory (via 'make test')."
    exit 1
fi

echo "🧪 --- Starting Automated MySCP Test ---"

# 1. Setup absolute paths so the SSH session doesn't get lost
PROJECT_ROOT=$(pwd)
SOURCE_FILE="$PROJECT_ROOT/test/test_payload.bin"
SANDBOX_DIR="$PROJECT_ROOT/test/sandbox"

# 2. Create the sandbox and generate a fresh 5MB dummy file
mkdir -p "$SANDBOX_DIR"
echo "[TEST] Generating 5MB payload..."
dd if=/dev/urandom of="$SOURCE_FILE" bs=1m count=5 2>/dev/null

# 3. Execute the transfer, routing it explicitly into the sandbox
echo "[TEST] Executing myscp-send over localhost..."
myscp-send 127.0.0.1 "$SOURCE_FILE" "$SANDBOX_DIR/"

# 4. Verify the transfer
if [ -f "$SANDBOX_DIR/test_payload.bin" ]; then
    echo "✅ [SUCCESS] File successfully streamed to isolated sandbox: $SANDBOX_DIR/test_payload.bin"
    exit 0
else
    echo "❌ [FAILED] File was not found in the sandbox!"
    exit 1
fi
