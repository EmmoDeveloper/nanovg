#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(binding = 0) uniform ViewUniforms {
	vec2 viewSize;
} view;

void main() {
	fragTexCoord = inTexCoord;

	// Transform to NDC - NanoVG uses Y-down (0 at top)
	vec2 ndc = (2.0 * inPos / view.viewSize) - 1.0;
	// Don't flip Y - keep NanoVG Y-down convention
	gl_Position = vec4(ndc.x, ndc.y, 0.0, 1.0);
}
