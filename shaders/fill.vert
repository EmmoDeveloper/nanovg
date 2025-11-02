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

	// Transform to NDC: (0,0) top-left -> (-1,-1), (viewSize.x, viewSize.y) bottom-right -> (1,1)
	// Match text shader convention: Y increases downward
	gl_Position = vec4(
		2.0 * inPosition.x / pc.viewSize.x - 1.0,
		2.0 * inPosition.y / pc.viewSize.y - 1.0,
		0.0,
		1.0
	);
}
