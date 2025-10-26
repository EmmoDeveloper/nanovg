#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
	vec2 screenSize;
} pc;

void main() {
	// Convert screen coordinates to NDC
	vec2 pos = inPosition / pc.screenSize * 2.0 - 1.0;
	gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
	fragColor = inColor;
}
