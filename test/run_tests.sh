#!/bin/bash

echo "========================================"
echo " Starting myscp SSH Integration Tests"
echo "========================================"

cd ..
make clean && make
make install
cd test

echo "[SETUP] Testing localhost SSH access..."
if ! ssh -o BatchMode=yes -o ConnectTimeout=2 127.0.0.1 exit 2>/dev/null; then
    echo "❌ [ERROR] Cannot SSH into 127.0.0.1."
    exit 1
fi

PAYLOAD="test_payload.bin"
echo "[TEST] Creating 5MB test payload..."
dd if=/dev/urandom of=$PAYLOAD bs=1M count=5 2>/dev/null

echo "----------------------------------------"
echo "[TEST 1] Valid Transfer to Home Directory"
~/.local/bin/myscp-send 127.0.0.1 $PAYLOAD

echo "----------------------------------------"
echo "[TEST 2] Path Parsing: Custom Filename"
~/.local/bin/myscp-send 127.0.0.1 $PAYLOAD custom_name.bin

echo "----------------------------------------"
echo "[TEST 3] Error Propagation: Invalid Path"
# This path does not exist, the receiver should fail, and the sender should catch it
~/.local/bin/myscp-send 127.0.0.1 $PAYLOAD /this/path/does/not/exist/

echo "----------------------------------------"
echo "[CLEANUP] Removing test artifacts..."
rm -f $PAYLOAD
rm -f ~/$PAYLOAD 
rm -f ~/custom_name.bin

echo "========================================"
echo " Tests Complete."
echo "========================================"
