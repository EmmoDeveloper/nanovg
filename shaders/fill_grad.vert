#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec2 fragPos;

layout(binding = 0) uniform ViewUniforms {
	vec2 viewSize;
} view;

void main() {
	fragTexCoord = inTexCoord;
	fragPos = inPos;

	// Transform to NDC - Vulkan Y-up convention
	vec2 ndc = (2.0 * inPos / view.viewSize) - 1.0;
	ndc.y = -ndc.y; // Flip Y for Vulkan Y-up (bottom-left origin)
	gl_Position = vec4(ndc.x, ndc.y, 0.0, 1.0);
}
