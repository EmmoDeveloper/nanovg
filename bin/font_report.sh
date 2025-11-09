#!/bin/bash
# Font Report Tool for NanoVG Project
# Analyzes all fonts in the fonts/ directory

set -e

FONTS_DIR="${1:-fonts}"
OUTPUT_FILE="font_report.txt"

if [ ! -d "$FONTS_DIR" ]; then
	echo "Error: Fonts directory '$FONTS_DIR' not found"
	exit 1
fi

echo "==================================="
echo "NanoVG Font Report"
echo "==================================="
echo "Generated: $(date)"
echo "Directory: $FONTS_DIR"
echo "==================================="
echo ""

# Summary
echo "SUMMARY"
echo "-------"
echo "Total font files:"
find "$FONTS_DIR" -type f \( -name "*.ttf" -o -name "*.otf" \) | wc -l
echo ""
echo "By category:"
for dir in "$FONTS_DIR"/*; do
	if [ -d "$dir" ]; then
		category=$(basename "$dir")
		count=$(find "$dir" -type f \( -name "*.ttf" -o -name "*.otf" \) 2>/dev/null | wc -l)
		if [ "$count" -gt 0 ]; then
			printf "  %-15s: %d fonts\n" "$category" "$count"
		fi
	fi
done
echo ""

# Directory structure
echo "DIRECTORY STRUCTURE"
echo "-------------------"
tree "$FONTS_DIR" -L 3 --dirsfirst
echo ""

# Detailed font information
echo "DETAILED FONT INFORMATION"
echo "-------------------------"
echo ""

find "$FONTS_DIR" -type f \( -name "*.ttf" -o -name "*.otf" \) | sort | while read -r font; do
	rel_path="${font#$FONTS_DIR/}"

	echo "Font: $rel_path"
	echo "  Path: $font"
	echo -n "  Size: "
	du -h "$font" | cut -f1

	# Use fc-query to get font information if available
	if command -v fc-query >/dev/null 2>&1; then
		family=$(fc-query -f "%{family}\n" "$font" 2>/dev/null | head -1)
		style=$(fc-query -f "%{style}\n" "$font" 2>/dev/null | head -1)

		[ -n "$family" ] && echo "  Family: $family"
		[ -n "$style" ] && echo "  Style: $style"

		# Check if it's a variable font
		if fc-query "$font" 2>/dev/null | grep -q "variable"; then
			echo "  Type: Variable Font"
		fi
	fi

	# Check for OpenType features using otfinfo if available
	if command -v otfinfo >/dev/null 2>&1; then
		features=$(otfinfo -f "$font" 2>/dev/null | wc -l)
		if [ "$features" -gt 0 ]; then
			echo "  OpenType Features: $features"
		fi
	fi

	echo ""
done

# Variable fonts section
echo "VARIABLE FONTS"
echo "--------------"
find "$FONTS_DIR/variable" -type f -name "*.ttf" 2>/dev/null | sort | while read -r font; do
	rel_path="${font#$FONTS_DIR/}"
	echo "  $rel_path"
done
echo ""

# Usage examples
echo "USAGE IN CODE"
echo "-------------"
echo "For dump_atlas.c and other tools, use these paths:"
echo ""
find "$FONTS_DIR" -type f \( -name "*.ttf" -o -name "*.otf" \) -not -path "*/static/*" | sort | while read -r font; do
	rel_path="${font#$FONTS_DIR/}"
	printf '  #define FONT_PATH "fonts/%s"\n' "$rel_path"
done
echo ""

echo "==================================="
echo "Report complete"
echo "==================================="
