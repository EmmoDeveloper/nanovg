#version 450

// Instance input (per glyph)
layout(location = 2) in vec2 inInstancePosition;  // Glyph screen position
layout(location = 3) in vec2 inInstanceSize;      // Glyph size
layout(location = 4) in vec2 inInstanceUVOffset;  // Atlas UV offset
layout(location = 5) in vec2 inInstanceUVSize;    // Atlas UV size

// Vertex output
layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec2 outPosition;

// Push constants for view transformation
layout(push_constant) uniform PushConstants {
	vec2 viewSize;
} pc;

void main() {
	// Generate quad corners from gl_VertexIndex
	// 0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
	// Using triangle strip ordering: 0, 1, 2, 3
	vec2 corners[4] = vec2[](
		vec2(0.0, 0.0),  // Top-left
		vec2(1.0, 0.0),  // Top-right
		vec2(0.0, 1.0),  // Bottom-left
		vec2(1.0, 1.0)   // Bottom-right
	);

	vec2 corner = corners[gl_VertexIndex];

	// Calculate vertex position
	vec2 position = inInstancePosition + corner * inInstanceSize;
	outPosition = position;

	// Calculate UV coordinates
	outTexCoord = inInstanceUVOffset + corner * inInstanceUVSize;

	// Transform to NDC with native Vulkan conventions (Y=0 at top)
	gl_Position = vec4(
		2.0 * position.x / pc.viewSize.x - 1.0,
		2.0 * position.y / pc.viewSize.y - 1.0,
		0.0,
		1.0
	);
}
