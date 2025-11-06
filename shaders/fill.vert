#version 450

// Vertex input
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

// Vertex output
layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec2 outPosition;

// Push constants for view transformation
layout(push_constant) uniform PushConstants {
	vec2 viewSize;
} pc;

void main() {
	outTexCoord = inTexCoord;
	outPosition = inPosition;

	// Transform to NDC: (0,0) bottom-left -> (-1,-1), (viewSize.x, viewSize.y) top-right -> (1,1)
	// Vulkan convention: Y increases upward
	gl_Position = vec4(
		2.0 * inPosition.x / pc.viewSize.x - 1.0,
		1.0 - 2.0 * inPosition.y / pc.viewSize.y,
		0.0,
		1.0
	);
}
