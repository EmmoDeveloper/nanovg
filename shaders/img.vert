#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(binding = 0) uniform ViewUniforms {
	vec2 viewSize;
} view;

void main() {
	fragTexCoord = inTexCoord;

	// Transform to NDC
	// Note: NanoVG provides coordinates with Y=0 at bottom (OpenGL convention)
	// But Vulkan uses Y=0 at top, so we negate Y to flip it
	vec2 ndc = (2.0 * inPos / view.viewSize) - 1.0;
	gl_Position = vec4(ndc.x, ndc.y, 0.0, 1.0);
}
