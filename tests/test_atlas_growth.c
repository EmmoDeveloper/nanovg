#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nanovg/nanovg.h"
#include "backends/vulkan/nvg_vk.h"
#include "../src/tools/window_utils.h"

int main(void) {
	printf("=== Atlas Growth Test ===\n\n");

	WindowVulkanContext* winCtx = window_create_context(1200, 900, "Atlas Growth Test");
	if (!winCtx) return 1;

	NVGcontext* vg = nvgCreateVk(winCtx->device, winCtx->physicalDevice,
	                              winCtx->graphicsQueue, winCtx->commandPool,
	                              winCtx->renderPass, 0);
	if (!vg) {
		window_destroy_context(winCtx);
		return 1;
	}

	// Load regular sans font
	int sansFont = nvgCreateFont(vg, "sans", "fonts/sans/NotoSans-Regular.ttf");
	if (sansFont == -1) {
		printf("Failed to load sans font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("Sans font loaded\n");

	// Load emoji font
	int emojiFont = nvgCreateFont(vg, "emoji", "fonts/emoji/Noto-COLRv1.ttf");
	if (emojiFont == -1) {
		printf("Failed to load emoji font\n");
		nvgDeleteVk(vg);
		window_destroy_context(winCtx);
		return 1;
	}
	printf("Emoji font loaded\n");

	// Render
	uint32_t imageIndex;
	vkAcquireNextImageKHR(winCtx->device, winCtx->swapchain, UINT64_MAX,
	                      winCtx->imageAvailableSemaphores[winCtx->currentFrame],
	                      VK_NULL_HANDLE, &imageIndex);

	VkCommandBuffer cmd = nvgVkGetCommandBuffer(vg);
	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = winCtx->renderPass;
	renderPassInfo.framebuffer = winCtx->framebuffers[imageIndex];
	renderPassInfo.renderArea.extent = winCtx->swapchainExtent;
	VkClearValue clearValues[2];
	clearValues[0].color = (VkClearColorValue){{1.0f, 1.0f, 1.0f, 1.0f}};
	clearValues[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0, 0, 1200, 900, 0, 1};
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = {{0, 0}, winCtx->swapchainExtent};
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	nvgVkBeginRenderPass(vg, &renderPassInfo, viewport, scissor);

	nvgBeginFrame(vg, 1200, 900, 1.0f);

	// Clear background
	nvgBeginPath(vg);
	nvgRect(vg, 0, 0, 1200, 900);
	nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
	nvgFill(vg);

	// Title
	nvgFontFace(vg, "sans");
	nvgFontSize(vg, 24.0f);
	nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgText(vg, 10, 30, "Atlas Growth Test - Many Glyphs", NULL);

	printf("Rendering many different characters to force atlas growth...\n");

	// Render lots of different ASCII characters
	nvgFontFace(vg, "sans");
	nvgFontSize(vg, 16.0f);
	float y = 60;
	int charCount = 0;

	// ASCII printable characters
	for (int row = 0; row < 6; row++) {
		char buffer[128];
		int idx = 0;
		for (int i = 0; i < 16; i++) {
			int c = 32 + row * 16 + i;
			if (c < 127) {
				buffer[idx++] = (char)c;
				charCount++;
			}
		}
		buffer[idx] = '\0';
		nvgText(vg, 10, y, buffer, NULL);
		y += 20;
	}

	printf("Rendered %d ASCII characters\n", charCount);

	// Render many different emojis to force RGBA atlas growth
	nvgFontFace(vg, "emoji");
	nvgFontSize(vg, 32.0f);
	y = 200;

	// Array of emoji codepoints to render - expanded to force atlas growth
	const char* emojis[] = {
		// Smileys & Emotion (U+1F600 - U+1F64F)
		"\xF0\x9F\x98\x80",  // U+1F600 GRINNING FACE
		"\xF0\x9F\x98\x81",  // U+1F601 GRINNING FACE WITH SMILING EYES
		"\xF0\x9F\x98\x82",  // U+1F602 FACE WITH TEARS OF JOY
		"\xF0\x9F\x98\x83",  // U+1F603 SMILING FACE WITH OPEN MOUTH
		"\xF0\x9F\x98\x84",  // U+1F604 SMILING FACE WITH OPEN MOUTH AND SMILING EYES
		"\xF0\x9F\x98\x85",  // U+1F605 SMILING FACE WITH OPEN MOUTH AND COLD SWEAT
		"\xF0\x9F\x98\x86",  // U+1F606 SMILING FACE WITH OPEN MOUTH AND TIGHTLY-CLOSED EYES
		"\xF0\x9F\x98\x87",  // U+1F607 SMILING FACE WITH HALO
		"\xF0\x9F\x98\x88",  // U+1F608 SMILING FACE WITH HORNS
		"\xF0\x9F\x98\x89",  // U+1F609 WINKING FACE
		"\xF0\x9F\x98\x8A",  // U+1F60A SMILING FACE WITH SMILING EYES
		"\xF0\x9F\x98\x8B",  // U+1F60B FACE SAVOURING DELICIOUS FOOD
		"\xF0\x9F\x98\x8C",  // U+1F60C RELIEVED FACE
		"\xF0\x9F\x98\x8D",  // U+1F60D SMILING FACE WITH HEART-SHAPED EYES
		"\xF0\x9F\x98\x8E",  // U+1F60E SMILING FACE WITH SUNGLASSES
		"\xF0\x9F\x98\x8F",  // U+1F60F SMIRKING FACE
		"\xF0\x9F\x98\x90",  // U+1F610 NEUTRAL FACE
		"\xF0\x9F\x98\x91",  // U+1F611 EXPRESSIONLESS FACE
		"\xF0\x9F\x98\x92",  // U+1F612 UNAMUSED FACE
		"\xF0\x9F\x98\x93",  // U+1F613 FACE WITH COLD SWEAT
		"\xF0\x9F\x98\x94",  // U+1F614 PENSIVE FACE
		"\xF0\x9F\x98\x95",  // U+1F615 CONFUSED FACE
		"\xF0\x9F\x98\x96",  // U+1F616 CONFOUNDED FACE
		"\xF0\x9F\x98\x97",  // U+1F617 KISSING FACE
		"\xF0\x9F\x98\x98",  // U+1F618 FACE THROWING A KISS
		"\xF0\x9F\x98\x99",  // U+1F619 KISSING FACE WITH SMILING EYES
		"\xF0\x9F\x98\x9A",  // U+1F61A KISSING FACE WITH CLOSED EYES
		"\xF0\x9F\x98\x9B",  // U+1F61B FACE WITH STUCK-OUT TONGUE
		"\xF0\x9F\x98\x9C",  // U+1F61C FACE WITH STUCK-OUT TONGUE AND WINKING EYE
		"\xF0\x9F\x98\x9D",  // U+1F61D FACE WITH STUCK-OUT TONGUE AND TIGHTLY-CLOSED EYES
		"\xF0\x9F\x98\x9E",  // U+1F61E DISAPPOINTED FACE
		"\xF0\x9F\x98\x9F",  // U+1F61F WORRIED FACE
		"\xF0\x9F\x98\xA0",  // U+1F620 ANGRY FACE
		"\xF0\x9F\x98\xA1",  // U+1F621 POUTING FACE
		"\xF0\x9F\x98\xA2",  // U+1F622 CRYING FACE
		"\xF0\x9F\x98\xA3",  // U+1F623 PERSEVERING FACE
		"\xF0\x9F\x98\xA4",  // U+1F624 FACE WITH LOOK OF TRIUMPH
		"\xF0\x9F\x98\xA5",  // U+1F625 DISAPPOINTED BUT RELIEVED FACE
		"\xF0\x9F\x98\xA6",  // U+1F626 FROWNING FACE WITH OPEN MOUTH
		"\xF0\x9F\x98\xA7",  // U+1F627 ANGUISHED FACE
		"\xF0\x9F\x98\xA8",  // U+1F628 FEARFUL FACE
		"\xF0\x9F\x98\xA9",  // U+1F629 WEARY FACE
		"\xF0\x9F\x98\xAA",  // U+1F62A SLEEPY FACE
		"\xF0\x9F\x98\xAB",  // U+1F62B TIRED FACE
		"\xF0\x9F\x98\xAC",  // U+1F62C GRIMACING FACE
		"\xF0\x9F\x98\xAD",  // U+1F62D LOUDLY CRYING FACE
		"\xF0\x9F\x98\xAE",  // U+1F62E FACE WITH OPEN MOUTH
		"\xF0\x9F\x98\xAF",  // U+1F62F HUSHED FACE
		"\xF0\x9F\x98\xB0",  // U+1F630 FACE WITH OPEN MOUTH AND COLD SWEAT
		"\xF0\x9F\x98\xB1",  // U+1F631 FACE SCREAMING IN FEAR
		"\xF0\x9F\x98\xB2",  // U+1F632 ASTONISHED FACE
		"\xF0\x9F\x98\xB3",  // U+1F633 FLUSHED FACE
		"\xF0\x9F\x98\xB4",  // U+1F634 SLEEPING FACE
		"\xF0\x9F\x98\xB5",  // U+1F635 DIZZY FACE
		"\xF0\x9F\x98\xB6",  // U+1F636 FACE WITHOUT MOUTH
		"\xF0\x9F\x98\xB7",  // U+1F637 FACE WITH MEDICAL MASK
		// Hearts and symbols
		"\xE2\x9D\xA4\xEF\xB8\x8F",  // U+2764 RED HEART
		"\xF0\x9F\x92\x99",  // U+1F499 BLUE HEART
		"\xF0\x9F\x92\x9A",  // U+1F49A GREEN HEART
		"\xF0\x9F\x92\x9B",  // U+1F49B YELLOW HEART
		"\xF0\x9F\x92\x9C",  // U+1F49C PURPLE HEART
		"\xF0\x9F\x92\x9D",  // U+1F49D HEART WITH RIBBON
		"\xF0\x9F\x92\x9E",  // U+1F49E REVOLVING HEARTS
		"\xF0\x9F\x92\x9F",  // U+1F49F HEART DECORATION
		// Animals & Nature
		"\xF0\x9F\x90\xB6",  // U+1F436 DOG FACE
		"\xF0\x9F\x90\xB1",  // U+1F431 CAT FACE
		"\xF0\x9F\x90\xAD",  // U+1F42D MOUSE FACE
		"\xF0\x9F\x90\xB9",  // U+1F439 HAMSTER FACE
		"\xF0\x9F\x90\xB0",  // U+1F430 RABBIT FACE
		"\xF0\x9F\x90\xBB",  // U+1F43B BEAR FACE
		"\xF0\x9F\x90\xBC",  // U+1F43C PANDA FACE
		"\xF0\x9F\x90\xA8",  // U+1F428 KOALA
		"\xF0\x9F\x90\xAF",  // U+1F42F TIGER FACE
		"\xF0\x9F\xA6\x81",  // U+1F981 LION FACE
		"\xF0\x9F\x90\xAE",  // U+1F42E COW FACE
		"\xF0\x9F\x90\xB7",  // U+1F437 PIG FACE
		"\xF0\x9F\x90\xBD",  // U+1F43D PIG NOSE
		"\xF0\x9F\x90\xB8",  // U+1F438 FROG FACE
		"\xF0\x9F\x90\xB5",  // U+1F435 MONKEY FACE
		// Food & Drink
		"\xF0\x9F\x8D\x8E",  // U+1F34E RED APPLE
		"\xF0\x9F\x8D\x8A",  // U+1F34A TANGERINE
		"\xF0\x9F\x8D\x8B",  // U+1F34B LEMON
		"\xF0\x9F\x8D\x8C",  // U+1F34C BANANA
		"\xF0\x9F\x8D\x89",  // U+1F349 WATERMELON
		"\xF0\x9F\x8D\x87",  // U+1F347 GRAPES
		"\xF0\x9F\x8D\x93",  // U+1F353 STRAWBERRY
		"\xF0\x9F\x8D\x92",  // U+1F352 CHERRIES
		"\xF0\x9F\x8D\x91",  // U+1F351 PEACH
		"\xF0\x9F\x8D\x8D",  // U+1F34D PINEAPPLE
		"\xF0\x9F\xA5\xA5",  // U+1F965 COCONUT
		"\xF0\x9F\xA5\x9D",  // U+1F95D KIWI FRUIT
		"\xF0\x9F\x8D\x85",  // U+1F345 TOMATO
		"\xF0\x9F\x8D\x86",  // U+1F346 EGGPLANT
		"\xF0\x9F\xA5\x91",  // U+1F951 AVOCADO
		"\xF0\x9F\xA5\x95",  // U+1F955 CARROT
		"\xF0\x9F\x8C\xBD",  // U+1F33D EAR OF CORN
		"\xF0\x9F\x8C\xB6\xEF\xB8\x8F",  // U+1F336 HOT PEPPER
		"\xF0\x9F\xA5\x92",  // U+1F952 CUCUMBER
		"\xF0\x9F\xA5\xAC",  // U+1F96C LEAFY GREEN
		"\xF0\x9F\xA5\xA6",  // U+1F966 BROCCOLI
		"\xF0\x9F\xA7\x84",  // U+1F9C4 GARLIC
		"\xF0\x9F\xA7\x85",  // U+1F9C5 ONION
		"\xF0\x9F\x8D\x84",  // U+1F344 MUSHROOM
		"\xF0\x9F\xA5\x9C",  // U+1F95C PEANUTS
		"\xF0\x9F\x8C\xB0",  // U+1F330 CHESTNUT
		"\xF0\x9F\x8D\x9E",  // U+1F35E BREAD
		"\xF0\x9F\xA5\x90",  // U+1F950 CROISSANT
		"\xF0\x9F\xA5\x96",  // U+1F956 BAGUETTE BREAD
		"\xF0\x9F\xA5\xA8",  // U+1F968 PRETZEL
		"\xF0\x9F\xA5\xAF",  // U+1F96F BAGEL
		"\xF0\x9F\xA5\x9E",  // U+1F95E PANCAKES
		"\xF0\x9F\xA7\x87",  // U+1F9C7 WAFFLE
		"\xF0\x9F\xA7\x80",  // U+1F9C0 CHEESE WEDGE
		"\xF0\x9F\x8D\x96",  // U+1F356 MEAT ON BONE
		"\xF0\x9F\x8D\x97",  // U+1F357 POULTRY LEG
		"\xF0\x9F\xA5\xA9",  // U+1F969 CUT OF MEAT
		"\xF0\x9F\xA5\x93",  // U+1F953 BACON
		"\xF0\x9F\x8D\x94",  // U+1F354 HAMBURGER
		"\xF0\x9F\x8D\x9F",  // U+1F35F FRENCH FRIES
		"\xF0\x9F\x8D\x95",  // U+1F355 PIZZA
		"\xF0\x9F\x8C\xAD",  // U+1F32D HOT DOG
		"\xF0\x9F\xA5\xAA",  // U+1F96A SANDWICH
		"\xF0\x9F\x8C\xAE",  // U+1F32E TACO
		"\xF0\x9F\x8C\xAF",  // U+1F32F BURRITO
		"\xF0\x9F\xA5\x99",  // U+1F959 STUFFED FLATBREAD
		"\xF0\x9F\xA7\x86",  // U+1F9C6 FALAFEL
		"\xF0\x9F\xA5\x9A",  // U+1F95A EGG
		"\xF0\x9F\x8D\xB3",  // U+1F373 COOKING
		"\xF0\x9F\xA5\x98",  // U+1F958 SHALLOW PAN OF FOOD
		"\xF0\x9F\x8D\xB2",  // U+1F372 POT OF FOOD
		"\xF0\x9F\xA5\xA3",  // U+1F963 BOWL WITH SPOON
		"\xF0\x9F\xA5\x97",  // U+1F957 GREEN SALAD
		"\xF0\x9F\x8D\xBF",  // U+1F37F POPCORN
		"\xF0\x9F\xA7\x88",  // U+1F9C8 BUTTER
		"\xF0\x9F\xA7\x82",  // U+1F9C2 SALT
		"\xF0\x9F\xA5\xAB",  // U+1F96B CANNED FOOD
		// More smileys for variety
		"\xF0\x9F\x98\xB8",  // U+1F638 GRINNING CAT FACE WITH SMILING EYES
		"\xF0\x9F\x98\xB9",  // U+1F639 CAT FACE WITH TEARS OF JOY
		"\xF0\x9F\x98\xBA",  // U+1F63A SMILING CAT FACE WITH OPEN MOUTH
		"\xF0\x9F\x98\xBB",  // U+1F63B SMILING CAT FACE WITH HEART-SHAPED EYES
		"\xF0\x9F\x98\xBC",  // U+1F63C CAT FACE WITH WRY SMILE
		"\xF0\x9F\x98\xBD",  // U+1F63D KISSING CAT FACE WITH CLOSED EYES
		"\xF0\x9F\x98\xBE",  // U+1F63E POUTING CAT FACE
		"\xF0\x9F\x98\xBF",  // U+1F63F CRYING CAT FACE
		"\xF0\x9F\x99\x80",  // U+1F640 WEARY CAT FACE
		"\xF0\x9F\x99\x88",  // U+1F648 SEE-NO-EVIL MONKEY
		"\xF0\x9F\x99\x89",  // U+1F649 HEAR-NO-EVIL MONKEY
		"\xF0\x9F\x99\x8A",  // U+1F64A SPEAK-NO-EVIL MONKEY
		"\xF0\x9F\x92\x8B",  // U+1F48B KISS MARK
		"\xF0\x9F\x92\x8C",  // U+1F48C LOVE LETTER
		"\xF0\x9F\x92\x98",  // U+1F498 HEART WITH ARROW
		"\xF0\x9F\x92\x96",  // U+1F496 SPARKLING HEART
		"\xF0\x9F\x92\x97",  // U+1F497 GROWING HEART
		"\xF0\x9F\x92\x93",  // U+1F493 BEATING HEART
		"\xF0\x9F\x92\x95",  // U+1F495 TWO HEARTS
		"\xF0\x9F\x92\x9E",  // U+1F49E REVOLVING HEARTS
		"\xF0\x9F\x92\x94",  // U+1F494 BROKEN HEART
		"\xE2\x9D\xA3\xEF\xB8\x8F",  // U+2763 HEAVY HEART EXCLAMATION MARK ORNAMENT
		"\xF0\x9F\x92\xA2",  // U+1F4A2 ANGER SYMBOL
		"\xF0\x9F\x92\xA5",  // U+1F4A5 COLLISION
		"\xF0\x9F\x92\xA6",  // U+1F4A6 SPLASHING SWEAT SYMBOL
		"\xF0\x9F\x92\xA8",  // U+1F4A8 DASH SYMBOL
		"\xF0\x9F\x92\xAB",  // U+1F4AB DIZZY SYMBOL
		"\xF0\x9F\x92\xAC",  // U+1F4AC SPEECH BALLOON
		"\xF0\x9F\x97\xA8\xEF\xB8\x8F",  // U+1F5E8 LEFT SPEECH BUBBLE
		"\xF0\x9F\x97\xAF\xEF\xB8\x8F",  // U+1F5EF RIGHT ANGER BUBBLE
		"\xF0\x9F\x92\xAD",  // U+1F4AD THOUGHT BALLOON
		"\xF0\x9F\x92\xA4",  // U+1F4A4 SLEEPING SYMBOL
	};

	int emojiCount = sizeof(emojis) / sizeof(emojis[0]);
	printf("Rendering %d different emojis...\n", emojiCount);

	float x = 10;
	int perRow = 20;  // More emojis per row for larger window
	for (int i = 0; i < emojiCount; i++) {
		if (i > 0 && i % perRow == 0) {
			x = 10;
			y += 40;
		}
		nvgText(vg, x, y, emojis[i], NULL);
		x += 50;  // Increased spacing
	}

	printf("Total unique glyphs rendered: ~%d\n", charCount + emojiCount);

	// Add a summary at the bottom
	nvgFontFace(vg, "sans");
	nvgFontSize(vg, 14.0f);
	nvgFillColor(vg, nvgRGBA(100, 100, 100, 255));
	char summary[128];
	snprintf(summary, sizeof(summary), "ASCII chars: %d  |  Emojis: %d  |  Total: %d unique glyphs",
	         charCount, emojiCount, charCount + emojiCount);
	nvgText(vg, 10, 880, summary, NULL);

	nvgEndFrame(vg);

	// Dump atlases for debugging
	printf("\nDumping atlas textures...\n");
	nvgVkDumpAtlasByFormat(vg, VK_FORMAT_R8_UNORM, "screendumps/atlas_alpha.ppm");
	nvgVkDumpAtlasByFormat(vg, VK_FORMAT_R8G8B8A8_UNORM, "screendumps/atlas_rgba.ppm");

	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);

	// Submit and present
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = {winCtx->imageAvailableSemaphores[winCtx->currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	VkSemaphore signalSemaphores[] = {winCtx->renderFinishedSemaphores[winCtx->currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkQueueSubmit(winCtx->graphicsQueue, 1, &submitInfo, winCtx->inFlightFences[winCtx->currentFrame]);

	// Save screenshot
	printf("Saving screenshot...\n");
	if (window_save_screenshot(winCtx, imageIndex, "screendumps/atlas_growth_test.ppm")) {
		printf("Screenshot saved to screendumps/atlas_growth_test.ppm\n");
	} else {
		printf("Failed to save screenshot\n");
	}

	// Cleanup
	vkDeviceWaitIdle(winCtx->device);
	nvgDeleteVk(vg);
	window_destroy_context(winCtx);

	printf("\nAtlas growth test completed successfully!\n");
	printf("If atlas growth occurred, you should see log messages above.\n");
	return 0;
}
