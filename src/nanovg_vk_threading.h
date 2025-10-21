//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
// Copyright (c) 2025 NanoVG Vulkan Backend Contributors
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef NANOVG_VK_THREADING_H
#define NANOVG_VK_THREADING_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "nanovg_vk_internal.h"

static void* vknvg__workerThread(void* arg)
{
	VKNVGthreadContext* ctx = (VKNVGthreadContext*)arg;
	VKNVGcontext* vk = ctx->vk;
	VKNVGworkQueue* queue = &vk->workQueue;

	while (!ctx->shouldExit) {
		pthread_mutex_lock(&queue->mutex);

		while (!queue->workSubmitted && !ctx->shouldExit) {
			pthread_cond_wait(&queue->workAvailable, &queue->mutex);
		}

		if (ctx->shouldExit) {
			pthread_mutex_unlock(&queue->mutex);
			break;
		}

		queue->activeWorkers++;
		pthread_mutex_unlock(&queue->mutex);

		while (1) {
			pthread_mutex_lock(&queue->mutex);
			uint32_t callIndex = queue->nextCallIndex;
			if (callIndex >= queue->callCount) {
				pthread_mutex_unlock(&queue->mutex);
				break;
			}
			queue->nextCallIndex++;
			pthread_mutex_unlock(&queue->mutex);

			(void)ctx; // TODO: Record actual draw commands
			// VkCommandBuffer cmd = ctx->secondaryCommandBuffers[vk->currentFrame];
		}

		pthread_mutex_lock(&queue->mutex);
		queue->activeWorkers--;
		if (queue->activeWorkers == 0) {
			queue->workSubmitted = VK_FALSE;
			pthread_cond_signal(&queue->workComplete);
		}
		pthread_mutex_unlock(&queue->mutex);
	}

	return NULL;
}

static int vknvg__initWorkQueue(VKNVGworkQueue* queue)
{
	memset(queue, 0, sizeof(VKNVGworkQueue));
	pthread_mutex_init(&queue->mutex, NULL);
	pthread_cond_init(&queue->workAvailable, NULL);
	pthread_cond_init(&queue->workComplete, NULL);
	return 1;
}

static void vknvg__destroyWorkQueue(VKNVGworkQueue* queue)
{
	pthread_mutex_destroy(&queue->mutex);
	pthread_cond_destroy(&queue->workAvailable);
	pthread_cond_destroy(&queue->workComplete);
}

static int vknvg__initThreadPool(VKNVGcontext* vk)
{
	if (vk->threadCount == 0) return 1;

	vk->threadContexts = (VKNVGthreadContext*)malloc(sizeof(VKNVGthreadContext) * vk->threadCount);
	if (!vk->threadContexts) return 0;

	if (!vknvg__initWorkQueue(&vk->workQueue)) {
		free(vk->threadContexts);
		return 0;
	}

	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = vk->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandBufferCount = vk->maxFrames;

	for (uint32_t i = 0; i < vk->threadCount; i++) {
		VKNVGthreadContext* ctx = &vk->threadContexts[i];
		ctx->threadIndex = i;
		ctx->vk = vk;
		ctx->active = VK_TRUE;
		ctx->shouldExit = VK_FALSE;

		ctx->secondaryCommandBuffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * vk->maxFrames);
		if (!ctx->secondaryCommandBuffers) {
			for (uint32_t j = 0; j < i; j++) {
				free(vk->threadContexts[j].secondaryCommandBuffers);
			}
			free(vk->threadContexts);
			vknvg__destroyWorkQueue(&vk->workQueue);
			return 0;
		}

		VkResult result = vkAllocateCommandBuffers(vk->device, &allocInfo, ctx->secondaryCommandBuffers);
		if (result != VK_SUCCESS) {
			free(ctx->secondaryCommandBuffers);
			for (uint32_t j = 0; j < i; j++) {
				vkFreeCommandBuffers(vk->device, vk->commandPool, vk->maxFrames, vk->threadContexts[j].secondaryCommandBuffers);
				free(vk->threadContexts[j].secondaryCommandBuffers);
			}
			free(vk->threadContexts);
			vknvg__destroyWorkQueue(&vk->workQueue);
			return 0;
		}

		pthread_create(&ctx->thread, NULL, vknvg__workerThread, ctx);
	}

	return 1;
}

static void vknvg__destroyThreadPool(VKNVGcontext* vk)
{
	if (vk->threadCount == 0 || vk->threadContexts == NULL) return;

	for (uint32_t i = 0; i < vk->threadCount; i++) {
		vk->threadContexts[i].shouldExit = VK_TRUE;
	}

	pthread_mutex_lock(&vk->workQueue.mutex);
	pthread_cond_broadcast(&vk->workQueue.workAvailable);
	pthread_mutex_unlock(&vk->workQueue.mutex);

	for (uint32_t i = 0; i < vk->threadCount; i++) {
		pthread_join(vk->threadContexts[i].thread, NULL);
	}

	for (uint32_t i = 0; i < vk->threadCount; i++) {
		vkFreeCommandBuffers(vk->device, vk->commandPool, vk->maxFrames, vk->threadContexts[i].secondaryCommandBuffers);
		free(vk->threadContexts[i].secondaryCommandBuffers);
	}

	vknvg__destroyWorkQueue(&vk->workQueue);
	free(vk->threadContexts);
	vk->threadContexts = NULL;
}

#endif // NANOVG_VK_THREADING_H
