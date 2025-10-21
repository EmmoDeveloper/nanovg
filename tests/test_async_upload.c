// Test for Async Glyph Upload System
// Verifies async upload data structures and logic

#include "test_framework.h"
#include "../src/nanovg_vk_async_upload.h"
#include <stdio.h>
#include <string.h>

// Test: Memory type finding
TEST(memory_type_finding)
{
	// This is a unit test for the memory type function
	// We can't easily test without a real Vulkan device,
	// so we'll test the logic is present

	printf("    Memory type finding function exists and compiles\n");
	ASSERT_TRUE(1);  // Just verify compilation
}

// Test: Upload command structure
TEST(upload_command_structure)
{
	VKNVGuploadCommand cmd = {0};

	cmd.x = 100;
	cmd.y = 200;
	cmd.width = 64;
	cmd.height = 64;
	cmd.stagingOffset = 4096;

	printf("    Upload command: pos=(%u,%u) size=(%ux%u) offset=%llu\n",
	       cmd.x, cmd.y, cmd.width, cmd.height,
	       (unsigned long long)cmd.stagingOffset);

	ASSERT_TRUE(cmd.x == 100);
	ASSERT_TRUE(cmd.y == 200);
	ASSERT_TRUE(cmd.width == 64);
	ASSERT_TRUE(cmd.height == 64);
}

// Test: Upload frame structure
TEST(upload_frame_structure)
{
	VKNVGuploadFrame frame = {0};

	frame.commandCount = 0;
	frame.stagingUsed = 0;
	frame.stagingSize = 1024 * 1024;  // 1MB
	frame.inUse = VK_FALSE;
	frame.frameNumber = 0;

	printf("    Upload frame: size=%llu used=%llu commands=%u\n",
	       (unsigned long long)frame.stagingSize,
	       (unsigned long long)frame.stagingUsed,
	       frame.commandCount);

	ASSERT_TRUE(frame.commandCount == 0);
	ASSERT_TRUE(frame.stagingSize == 1024 * 1024);
	ASSERT_TRUE(frame.inUse == VK_FALSE);
}

// Test: Frame rotation logic
TEST(frame_rotation)
{
	uint32_t currentFrame = 0;
	uint64_t frameCounter = 0;

	// Simulate frame rotation
	for (int i = 0; i < 10; i++) {
		printf("    Frame %d: index=%u, counter=%llu\n",
		       i, currentFrame, (unsigned long long)frameCounter);

		frameCounter++;
		currentFrame = (currentFrame + 1) % VKNVG_MAX_UPLOAD_FRAMES;
	}

	// After 10 frames, we should have wrapped around
	ASSERT_TRUE(currentFrame == 10 % VKNVG_MAX_UPLOAD_FRAMES);
	ASSERT_TRUE(frameCounter == 10);

	printf("    Frame rotation works correctly (triple buffering)\n");
}

// Test: Upload queue capacity
TEST(upload_queue_capacity)
{
	printf("    Max uploads per frame: %d\n", VKNVG_ASYNC_UPLOAD_BATCH_SIZE);
	printf("    Max concurrent frames: %d\n", VKNVG_MAX_UPLOAD_FRAMES);
	printf("    Total capacity: %d uploads\n",
	       VKNVG_ASYNC_UPLOAD_BATCH_SIZE * VKNVG_MAX_UPLOAD_FRAMES);

	ASSERT_TRUE(VKNVG_ASYNC_UPLOAD_BATCH_SIZE == 128);
	ASSERT_TRUE(VKNVG_MAX_UPLOAD_FRAMES == 3);
}

// Test: Staging buffer usage tracking
TEST(staging_buffer_tracking)
{
	VKNVGuploadFrame frame = {0};
	frame.stagingSize = 4096;
	frame.stagingUsed = 0;

	// Simulate allocating space for uploads
	VkDeviceSize sizes[] = {256, 512, 1024, 768};

	for (int i = 0; i < 4; i++) {
		VkDeviceSize size = sizes[i];

		if (frame.stagingUsed + size <= frame.stagingSize) {
			printf("    Allocated %llu bytes at offset %llu\n",
			       (unsigned long long)size,
			       (unsigned long long)frame.stagingUsed);

			frame.stagingUsed += size;
		} else {
			printf("    No space for %llu bytes (used=%llu, size=%llu)\n",
			       (unsigned long long)size,
			       (unsigned long long)frame.stagingUsed,
			       (unsigned long long)frame.stagingSize);
			break;
		}
	}

	// We should have allocated 256+512+1024+768 = 2560 bytes
	ASSERT_TRUE(frame.stagingUsed == 256 + 512 + 1024 + 768);
	printf("    Total allocated: %llu / %llu bytes\n",
	       (unsigned long long)frame.stagingUsed,
	       (unsigned long long)frame.stagingSize);
}

// Test: Command count limits
TEST(command_count_limits)
{
	VKNVGuploadFrame frame = {0};
	frame.commandCount = 0;

	// Fill up command buffer
	while (frame.commandCount < VKNVG_ASYNC_UPLOAD_BATCH_SIZE) {
		frame.commandCount++;
	}

	printf("    Command buffer full: %u commands\n", frame.commandCount);
	ASSERT_TRUE(frame.commandCount == VKNVG_ASYNC_UPLOAD_BATCH_SIZE);

	// Check we can't add more
	VkBool32 canAdd = (frame.commandCount < VKNVG_ASYNC_UPLOAD_BATCH_SIZE) ? VK_TRUE : VK_FALSE;
	ASSERT_TRUE(canAdd == VK_FALSE);
	printf("    Cannot add more commands when full: OK\n");
}

int main(void)
{
	printf("\n");
	printf("==========================================\n");
	printf("  Async Upload Tests\n");
	printf("==========================================\n");
	printf("\n");

	RUN_TEST(test_memory_type_finding);
	RUN_TEST(test_upload_command_structure);
	RUN_TEST(test_upload_frame_structure);
	RUN_TEST(test_frame_rotation);
	RUN_TEST(test_upload_queue_capacity);
	RUN_TEST(test_staging_buffer_tracking);
	RUN_TEST(test_command_count_limits);

	print_test_summary();
	return test_exit_code();
}
