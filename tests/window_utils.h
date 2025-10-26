#ifndef WINDOW_UTILS_H
#define WINDOW_UTILS_H

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Forward declarations
typedef struct NVGcontext NVGcontext;

typedef struct {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	uint32_t graphicsQueueFamilyIndex;
	uint32_t presentQueueFamilyIndex;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;
	VkDebugUtilsMessengerEXT debugMessenger;

	GLFWwindow* window;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	uint32_t swapchainImageCount;
	VkImage* swapchainImages;
	VkImageView* swapchainImageViews;
	VkFramebuffer* framebuffers;
	VkRenderPass renderPass;

	VkImage depthStencilImage;
	VkDeviceMemory depthStencilImageMemory;
	VkImageView depthStencilImageView;
	VkFormat depthStencilFormat;

	VkSemaphore* imageAvailableSemaphores;  // Per swapchain image
	VkSemaphore* renderFinishedSemaphores;  // Per swapchain image
	VkFence* inFlightFences;                // Per frame in flight
	VkFence* imageInFlightFences;           // Per swapchain image, tracks which frame is using it
	VkCommandBuffer* commandBuffers;        // Per frame in flight
	uint32_t currentFrame;
	uint32_t maxFramesInFlight;

	int width;
	int height;
	int framebufferResized;
} WindowVulkanContext;

WindowVulkanContext* window_create_context(int width, int height, const char* title);

void window_destroy_context(WindowVulkanContext* ctx);

NVGcontext* window_create_nanovg_context(WindowVulkanContext* ctx, int flags);

int window_begin_frame(WindowVulkanContext* ctx, uint32_t* imageIndex, VkCommandBuffer* cmdBuf);

void window_end_frame(WindowVulkanContext* ctx, uint32_t imageIndex, VkCommandBuffer cmdBuf);

int window_should_close(WindowVulkanContext* ctx);

void window_poll_events(void);

void window_get_framebuffer_size(WindowVulkanContext* ctx, int* width, int* height);

VkImageView window_get_swapchain_image_view(WindowVulkanContext* ctx, uint32_t imageIndex);

VkImageView window_get_depth_stencil_image_view(WindowVulkanContext* ctx);

int window_save_screenshot(WindowVulkanContext* ctx, uint32_t imageIndex, const char* filename);

#endif
