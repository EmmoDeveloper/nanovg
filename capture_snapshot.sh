#!/bin/bash
# Run test in background and capture stdout
./build/test_window 2>&1 | tee /tmp/test_output.txt &
PID=$!

# Wait for window to appear
sleep 0.5

# Wait for "Taking snapshot" message
timeout 5 bash -c 'while ! grep -q "Taking snapshot" /tmp/test_output.txt 2>/dev/null; do sleep 0.1; done' || true

# Find window
WINDOW_ID=$(xdotool search --name "NanoVG Vulkan Test" 2>/dev/null | tail -1)
if [ -z "$WINDOW_ID" ]; then
  WINDOW_ID=$(xdotool search --pid $PID 2>/dev/null | tail -1)
fi

if [ -n "$WINDOW_ID" ]; then
  echo "Found window: $WINDOW_ID"
  import -window $WINDOW_ID /tmp/nanovg_snapshot.png 2>/dev/null
  echo "Snapshot saved to /tmp/nanovg_snapshot.png"
  ls -lh /tmp/nanovg_snapshot.png
else
  echo "Failed to find window"
fi

wait $PID 2>/dev/null || true
