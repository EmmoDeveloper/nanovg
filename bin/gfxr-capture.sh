#!/bin/bash
# GFXReconstruct Capture Helper Script
#
# This script makes it easy to capture Vulkan API calls from your tests
# for debugging, profiling, and replay analysis.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GFXR_DIR="$SCRIPT_DIR/captures"
GFXR_LAYER="VK_LAYER_LUNARG_gfxreconstruct"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

show_help() {
	echo "GFXReconstruct Capture Helper"
	echo ""
	echo "Usage: $0 <command> [options]"
	echo ""
	echo "Commands:"
	echo "  capture <program> [args]  - Capture Vulkan calls from program"
	echo "  replay <file>             - Replay captured trace"
	echo "  info <file>               - Show capture file information"
	echo "  list                      - List all captures"
	echo "  clean                     - Remove all captures"
	echo ""
	echo "Examples:"
	echo "  $0 capture ./build/test_unit_texture"
	echo "  $0 replay captures/test_unit_texture_*.gfxr"
	echo "  $0 info captures/test_unit_texture_*.gfxr"
	echo ""
	echo "Environment variables (optional):"
	echo "  GFXR_CAPTURE_FRAMES=<start>-<end>  - Capture specific frame range"
	echo "  GFXR_LOG_LEVEL=<level>             - Set log level (debug, info, warning, error)"
	echo "  GFXR_MEMORY_TRACKING=true          - Enable memory tracking"
	echo ""
}

setup_capture_env() {
	mkdir -p "$GFXR_DIR"

	# Base capture filename from program name
	local program_name=$(basename "$1")
	local timestamp=$(date +%Y%m%d_%H%M%S)
	local capture_file="$GFXR_DIR/${program_name}_${timestamp}.gfxr"

	# Set up GFXReconstruct environment
	export VK_INSTANCE_LAYERS="$GFXR_LAYER"
	export VK_LAYER_PATH="/opt/lunarg-vulkan-sdk/x86_64/share/vulkan/explicit_layer.d"
	export VK_GFXRECON_CAPTURE_FILE="$capture_file"

	# Optional settings
	if [ -n "$GFXR_CAPTURE_FRAMES" ]; then
		export VK_GFXRECON_CAPTURE_FRAMES="$GFXR_CAPTURE_FRAMES"
	fi

	if [ -n "$GFXR_LOG_LEVEL" ]; then
		export VK_GFXRECON_LOG_LEVEL="$GFXR_LOG_LEVEL"
	fi

	if [ "$GFXR_MEMORY_TRACKING" = "true" ]; then
		export VK_GFXRECON_MEMORY_TRACKING_MODE="page_guard"
	fi

	echo "$capture_file"
}

cmd_capture() {
	if [ $# -lt 1 ]; then
		echo -e "${RED}Error: No program specified${NC}"
		echo "Usage: $0 capture <program> [args]"
		exit 1
	fi

	local program="$1"
	shift

	if [ ! -x "$program" ]; then
		echo -e "${RED}Error: Program not found or not executable: $program${NC}"
		exit 1
	fi

	echo -e "${GREEN}Setting up GFXReconstruct capture...${NC}"
	local capture_file=$(setup_capture_env "$program")

	echo -e "${GREEN}Capture file: $capture_file${NC}"
	echo -e "${YELLOW}Running: $program $@${NC}"
	echo ""

	# Run the program with capture enabled
	"$program" "$@"
	local exit_code=$?

	echo ""
	if [ $exit_code -eq 0 ]; then
		echo -e "${GREEN}✓ Capture complete: $capture_file${NC}"

		# Show file size
		local size=$(du -h "$capture_file" 2>/dev/null | cut -f1)
		echo -e "${GREEN}  File size: $size${NC}"

		# Quick info
		echo ""
		echo -e "${YELLOW}Quick info:${NC}"
		/opt/lunarg-vulkan-sdk/x86_64/bin/gfxrecon-info "$capture_file" 2>/dev/null | head -20 || true

		echo ""
		echo -e "${YELLOW}To replay:${NC}"
		echo "  $0 replay $capture_file"
	else
		echo -e "${RED}✗ Program exited with code: $exit_code${NC}"
		if [ -f "$capture_file" ]; then
			echo -e "${YELLOW}Partial capture saved: $capture_file${NC}"
		fi
	fi

	return $exit_code
}

cmd_replay() {
	if [ $# -lt 1 ]; then
		echo -e "${RED}Error: No capture file specified${NC}"
		echo "Usage: $0 replay <file>"
		exit 1
	fi

	local capture_file="$1"

	if [ ! -f "$capture_file" ]; then
		echo -e "${RED}Error: Capture file not found: $capture_file${NC}"
		exit 1
	fi

	echo -e "${GREEN}Replaying capture: $capture_file${NC}"
	/opt/lunarg-vulkan-sdk/x86_64/bin/gfxrecon-replay "$capture_file"
}

cmd_info() {
	if [ $# -lt 1 ]; then
		echo -e "${RED}Error: No capture file specified${NC}"
		echo "Usage: $0 info <file>"
		exit 1
	fi

	local capture_file="$1"

	if [ ! -f "$capture_file" ]; then
		echo -e "${RED}Error: Capture file not found: $capture_file${NC}"
		exit 1
	fi

	echo -e "${GREEN}Capture information: $capture_file${NC}"
	echo ""
	/opt/lunarg-vulkan-sdk/x86_64/bin/gfxrecon-info "$capture_file"
}

cmd_list() {
	if [ ! -d "$GFXR_DIR" ] || [ -z "$(ls -A "$GFXR_DIR" 2>/dev/null)" ]; then
		echo -e "${YELLOW}No captures found${NC}"
		return
	fi

	echo -e "${GREEN}Available captures:${NC}"
	echo ""

	for file in "$GFXR_DIR"/*.gfxr; do
		if [ -f "$file" ]; then
			local size=$(du -h "$file" | cut -f1)
			local name=$(basename "$file")
			echo -e "  ${YELLOW}$name${NC} ($size)"
		fi
	done
}

cmd_clean() {
	if [ ! -d "$GFXR_DIR" ] || [ -z "$(ls -A "$GFXR_DIR" 2>/dev/null)" ]; then
		echo -e "${YELLOW}No captures to clean${NC}"
		return
	fi

	echo -e "${YELLOW}Removing all captures...${NC}"
	rm -rf "$GFXR_DIR"
	echo -e "${GREEN}✓ All captures removed${NC}"
}

# Main command dispatcher
if [ $# -lt 1 ]; then
	show_help
	exit 1
fi

command="$1"
shift

case "$command" in
	capture)
		cmd_capture "$@"
		;;
	replay)
		cmd_replay "$@"
		;;
	info)
		cmd_info "$@"
		;;
	list)
		cmd_list
		;;
	clean)
		cmd_clean
		;;
	help|--help|-h)
		show_help
		;;
	*)
		echo -e "${RED}Error: Unknown command: $command${NC}"
		echo ""
		show_help
		exit 1
		;;
esac
