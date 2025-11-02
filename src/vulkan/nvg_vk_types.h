#ifndef NVG_VK_TYPES_H
#define NVG_VK_TYPES_H

#include <vulkan/vulkan.h>

// Include NanoVG for NVGvertex definition
#ifndef NANOVG_H
// Forward declare if nanovg.h not included yet
typedef struct NVGvertex NVGvertex;
#endif

// Forward declarations
typedef struct NVGVkContext NVGVkContext;
typedef struct NVGVkTexture NVGVkTexture;
typedef struct NVGVkBuffer NVGVkBuffer;
typedef struct NVGVkCall NVGVkCall;
typedef struct NVGVkUniforms NVGVkUniforms;
typedef struct NVGVkPipeline NVGVkPipeline;
typedef struct NVGVkShaderSet NVGVkShaderSet;

// Maximum limits
#define NVGVK_MAX_TEXTURES 256
#define NVGVK_MAX_CALLS 1024
#define NVGVK_INITIAL_VERTEX_COUNT 4096
#define NVGVK_INITIAL_INDEX_COUNT 8192
#define NVGVK_PIPELINE_COUNT 8

// Texture structure
struct NVGVkTexture {
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory memory;
	VkSampler sampler;
	VkDescriptorSet descriptorSet;  // Per-texture descriptor set
	int width;
	int height;
	int type;
	int flags;
};

// Buffer structure
struct NVGVkBuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* mapped;
	VkDeviceSize size;
	VkDeviceSize capacity;
	VkBufferUsageFlags usage;
};

// Shader uniform structure (matches shader layout)
struct NVGVkUniforms {
	float viewSize[2];
	float scissorMat[12];  // 3x4 matrix (padding for alignment)
	float paintMat[12];    // 3x4 matrix (padding for alignment)
	float innerCol[4];
	float outerCol[4];
	float scissorExt[2];
	float scissorScale[2];
	float extent[2];
	float radius;
	float feather;
	float strokeMult;
	float strokeThr;
	int texType;
	int type;
	int _padding[2];  // Explicit padding for alignment
};

// Call type enum
typedef enum NVGVkCallType {
	NVGVK_NONE = 0,
	NVGVK_FILL,
	NVGVK_CONVEXFILL,
	NVGVK_STROKE,
	NVGVK_TRIANGLES
} NVGVkCallType;

// Render call structure
struct NVGVkCall {
	NVGVkCallType type;
	int image;
	int pathOffset;
	int pathCount;
	int triangleOffset;
	int triangleCount;
	int uniformOffset;
	int blendFunc;
};

// Shader set (vertex + fragment)
struct NVGVkShaderSet {
	VkShaderModule vertShader;
	VkShaderModule fragShader;
};

// Pipeline structure
struct NVGVkPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
};

// Path structure
typedef struct NVGVkPath {
	int fillOffset;
	int fillCount;
	int strokeOffset;
	int strokeCount;
} NVGVkPath;

// Fragment uniform structure (for push constants or UBO)
typedef struct NVGVkFragUniforms {
	float scissorMat[12];
	float paintMat[12];
	float innerCol[4];
	float outerCol[4];
	float scissorExt[2];
	float scissorScale[2];
	float extent[2];
	float radius;
	float feather;
	float strokeMult;
	float strokeThr;
	int texType;
	int type;
} NVGVkFragUniforms;

// Main Vulkan context
struct NVGVkContext {
	// Vulkan handles (not owned)
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkQueue queue;
	VkCommandPool commandPool;

	// Owned resources
	VkCommandBuffer commandBuffer;
	VkFence uploadFence;  // Fence for texture upload synchronization

	// Render pass state (not owned, just tracked)
	VkRenderPass activeRenderPass;
	VkFramebuffer activeFramebuffer;
	VkRect2D renderArea;
	VkClearValue clearValues[2];
	uint32_t clearValueCount;
	VkViewport viewport;
	VkRect2D scissor;
	int inRenderPass;

	// Shaders
	NVGVkShaderSet shaders[NVGVK_PIPELINE_COUNT];

	// Pipelines
	NVGVkPipeline pipelines[NVGVK_PIPELINE_COUNT];
	int currentPipeline;

	// Buffers
	NVGVkBuffer vertexBuffer;
	NVGVkBuffer uniformBuffer;

	// Texture descriptor resources
	VkDescriptorSetLayout textureDescriptorSetLayout;
	VkDescriptorPool textureDescriptorPool;

	// Textures
	NVGVkTexture textures[NVGVK_MAX_TEXTURES];
	int textureCount;

	// Render state
	NVGVkCall calls[NVGVK_MAX_CALLS];
	int callCount;
	NVGVkPath paths[NVGVK_MAX_CALLS];
	int pathCount;
	NVGVkUniforms uniforms[NVGVK_MAX_CALLS];
	int uniformCount;

	// Vertex data
	float* vertices;
	int vertexCount;
	int vertexCapacity;

	// View state
	float viewWidth;
	float viewHeight;
	float devicePixelRatio;

	// Flags
	int flags;
	int edgeAntiAlias;

	// Virtual atlas (forward declared in nanovg_vk_virtual_atlas.h)
	void* virtualAtlas;  // VKNVGvirtualAtlas*
};

#endif // NVG_VK_TYPES_H
