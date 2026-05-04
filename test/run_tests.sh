#!/bin/bash

# Ensure we are in the test directory
cd "$(dirname "$0")"

# 1. Generate a random 5MB test file
echo "[TEST] Generating 5MB payload..."
dd if=/dev/urandom of=client/payload.bin bs=1024 count=5000 2>/dev/null

# 2. Start Receiver in the background
echo "[TEST] Starting receiver..."
cd server
./receiver > ../recv_log.txt 2>&1 &
RECV_PID=$!
cd ..

# Give the receiver a second to start listening
sleep 1 

# 3. Run the Sender
echo "[TEST] Running sender..."
cd client
./sender 127.0.0.1 payload.bin > ../send_log.txt 2>&1
cd ..

# Give the transfer a moment to finish writing to disk
sleep 1

# 4. Verify the results
echo "-----------------------------------"
if cmp -s "client/payload.bin" "server/payload.bin"; then
    echo "✅ [RESULT] SUCCESS: Files are completely identical!"
else
    echo "❌ [RESULT] FAILED: Files do not match!"
    echo "Check recv_log.txt and send_log.txt for details."
fi
echo "-----------------------------------"

# 5. Cleanup the background process
kill $RECV_PID 2>/dev/null
