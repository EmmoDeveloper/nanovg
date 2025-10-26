#include "window_utils.h"
#include "../src/vulkan/vk_shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Vertex data for a red rectangle
typedef struct {
	float pos[2];
	float color[3];
} Vertex;

static Vertex vertices[] = {
	{{100.0f, 100.0f}, {1.0f, 0.0f, 0.0f}},  // Top-left
	{{300.0f, 100.0f}, {1.0f, 0.0f, 0.0f}},  // Top-right
	{{300.0f, 300.0f}, {1.0f, 0.0f, 0.0f}},  // Bottom-right
	{{100.0f, 300.0f}, {1.0f, 0.0f, 0.0f}},  // Bottom-left
};

static uint16_t indices[] = {
	0, 1, 2,  // First triangle
	2, 3, 0   // Second triangle
};

int main(void)
{
	printf("Creating Vulkan window...\n");

	WindowVulkanContext* ctx = window_create_context(800, 600, "Test Window - Red Rectangle");
	if (!ctx) {
		fprintf(stderr, "Failed to create window context\n");
		return 1;
	}

	printf("Window created successfully!\n");

	// Create vertex buffer
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(vertices);
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer vertexBuffer;
	if (vkCreateBuffer(ctx->device, &bufferInfo, NULL, &vertexBuffer) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create vertex buffer\n");
		return 1;
	}

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(ctx->device, vertexBuffer, &memReq);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProps);

	uint32_t memTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((memReq.memoryTypeBits & (1 << i)) &&
			(memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
			(memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			memTypeIndex = i;
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = memTypeIndex;

	VkDeviceMemory vertexMemory;
	vkAllocateMemory(ctx->device, &allocInfo, NULL, &vertexMemory);
	vkBindBufferMemory(ctx->device, vertexBuffer, vertexMemory, 0);

	void* data;
	vkMapMemory(ctx->device, vertexMemory, 0, sizeof(vertices), 0, &data);
	memcpy(data, vertices, sizeof(vertices));
	vkUnmapMemory(ctx->device, vertexMemory);

	// Create index buffer
	bufferInfo.size = sizeof(indices);
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	VkBuffer indexBuffer;
	vkCreateBuffer(ctx->device, &bufferInfo, NULL, &indexBuffer);

	vkGetBufferMemoryRequirements(ctx->device, indexBuffer, &memReq);

	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = memTypeIndex;

	VkDeviceMemory indexMemory;
	vkAllocateMemory(ctx->device, &allocInfo, NULL, &indexMemory);
	vkBindBufferMemory(ctx->device, indexBuffer, indexMemory, 0);

	vkMapMemory(ctx->device, indexMemory, 0, sizeof(indices), 0, &data);
	memcpy(data, indices, sizeof(indices));
	vkUnmapMemory(ctx->device, indexMemory);

	// Load shaders
	VkShaderModule vertShader = vk_load_shader_module(ctx->device, "shaders/simple.vert.spv");
	VkShaderModule fragShader = vk_load_shader_module(ctx->device, "shaders/simple.frag.spv");

	if (!vertShader || !fragShader) {
		fprintf(stderr, "Failed to load shaders\n");
		return 1;
	}

	// Create pipeline layout
	VkPushConstantRange pushConstantRange = {0};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(float) * 2;  // vec2 screenSize

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	VkPipelineLayout pipelineLayout;
	vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &pipelineLayout);

	// Create graphics pipeline
	VkPipelineShaderStageCreateInfo vertStageInfo = {0};
	vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStageInfo.module = vertShader;
	vertStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragStageInfo = {0};
	fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStageInfo.module = fragShader;
	fragStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

	VkVertexInputBindingDescription bindingDesc = {0};
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(Vertex);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescs[2] = {0};
	attributeDescs[0].binding = 0;
	attributeDescs[0].location = 0;
	attributeDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescs[0].offset = offsetof(Vertex, pos);

	attributeDescs[1].binding = 0;
	attributeDescs[1].location = 1;
	attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescs[1].offset = offsetof(Vertex, color);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = 2;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)ctx->swapchainExtent.width;
	viewport.height = (float)ctx->swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {0};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = ctx->swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {0};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {0};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	VkPipelineMultisampleStateCreateInfo multisampling = {0};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
	                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {0};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkGraphicsPipelineCreateInfo pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = ctx->renderPass;
	pipelineInfo.subpass = 0;

	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create graphics pipeline\n");
		return 1;
	}

	printf("Pipeline created successfully!\n");
	printf("Rendering frames...\n");

	// Render loop
	int screenshotTaken = 0;
	for (int frame = 0; frame < 60; frame++) {
		uint32_t imageIndex;
		VkCommandBuffer cmd;
		if (!window_begin_frame(ctx, &imageIndex, &cmd)) {
			fprintf(stderr, "Failed to begin frame\n");
			break;
		}

		VkRenderPassBeginInfo renderPassInfo = {0};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = ctx->renderPass;
		renderPassInfo.framebuffer = ctx->framebuffers[imageIndex];
		renderPassInfo.renderArea.offset.x = 0;
		renderPassInfo.renderArea.offset.y = 0;
		renderPassInfo.renderArea.extent = ctx->swapchainExtent;

		VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind pipeline and draw
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Push constants (screen size)
		float screenSize[2] = {(float)ctx->swapchainExtent.width, (float)ctx->swapchainExtent.height};
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(screenSize), screenSize);

		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

		vkCmdEndRenderPass(cmd);

		window_end_frame(ctx, imageIndex, cmd);

		// Take screenshot on frame 10
		if (frame == 10 && !screenshotTaken) {
			printf("Taking screenshot...\n");
			if (window_save_screenshot(ctx, imageIndex, "/tmp/test-window.ppm")) {
				printf("Screenshot saved successfully\n");
				screenshotTaken = 1;
			}
		}

		if (window_should_close(ctx)) {
			break;
		}

		window_poll_events();
	}

	// Cleanup
	printf("Cleaning up...\n");
	vkDeviceWaitIdle(ctx->device);

	vkDestroyPipeline(ctx->device, pipeline, NULL);
	vkDestroyPipelineLayout(ctx->device, pipelineLayout, NULL);
	vkDestroyShaderModule(ctx->device, vertShader, NULL);
	vkDestroyShaderModule(ctx->device, fragShader, NULL);
	vkDestroyBuffer(ctx->device, vertexBuffer, NULL);
	vkFreeMemory(ctx->device, vertexMemory, NULL);
	vkDestroyBuffer(ctx->device, indexBuffer, NULL);
	vkFreeMemory(ctx->device, indexMemory, NULL);

	window_destroy_context(ctx);
	printf("Test completed successfully!\n");

	return 0;
}
