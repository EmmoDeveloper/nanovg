#!/bin/bash
#
# Convert SPIR-V binary to C header file
#

if [ $# -ne 2 ]; then
	echo "Usage: $0 <input.spv> <output.h>"
	exit 1
fi

INPUT="$1"
OUTPUT="$2"

if [ ! -f "$INPUT" ]; then
	echo "Error: Input file $INPUT not found"
	exit 1
fi

# Extract base name for array name
BASENAME=$(basename "$INPUT" .spv)
ARRAY_NAME="${BASENAME//./_}"

# Generate header file
xxd -i "$INPUT" > "$OUTPUT"

# Get the size
SIZE=$(stat -c%s "$INPUT")

echo "Generated $OUTPUT (${SIZE} bytes)"
