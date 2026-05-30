#pragma once
// VkFFT-based FFT Poisson solver — cross-vendor GPU (NVIDIA/AMD/Intel)
// Requires: Vulkan SDK 1.3+, VkFFT (subprojects/vkfft)
// Compile with meson -Duse_vkfft=true

#include "SimulationConfig.h"

#ifdef USE_VKFFT
#define VKFFT_BACKEND 0
#include "vulkan/vulkan.h"
#include "vkFFT.h"
#include "utils_VkFFT.h"
#include <vector>
#include <cstring>

struct CopiableSimulation::VkFFTSolver
{
	bool available = false;

	VkGPU vkGPU = {};
	VkFFTApplication appFwd = {};
	VkFFTApplication appInv = {};
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory bufferDeviceMemory = VK_NULL_HANDLE;
	uint64_t bufferSize = 0;

	std::vector<float> kernelCplx;

	int paddedW = 0, paddedH = 0;
	uint64_t totalSize = 0;

	~VkFFTSolver() { Release(); }

	void Release()
	{
		available = false;
		if (buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(vkGPU.device, buffer, nullptr);
			vkFreeMemory(vkGPU.device, bufferDeviceMemory, nullptr);
			buffer = VK_NULL_HANDLE;
		}
		deleteVkFFT(&appFwd);
		deleteVkFFT(&appInv);
		if (vkGPU.fence != VK_NULL_HANDLE) { vkDestroyFence(vkGPU.device, vkGPU.fence, nullptr); }
		if (vkGPU.commandPool != VK_NULL_HANDLE) { vkDestroyCommandPool(vkGPU.device, vkGPU.commandPool, nullptr); }
		if (vkGPU.device != VK_NULL_HANDLE) { vkDestroyDevice(vkGPU.device, nullptr); }
		if (vkGPU.instance != VK_NULL_HANDLE) { vkDestroyInstance(vkGPU.instance, nullptr); }
	}

	void Init(int w, int h)
	{
		paddedW = 3 * w; paddedH = 3 * h;
		totalSize = (uint64_t)paddedW * paddedH;

		vkGPU.enableValidationLayers = 0;
		if (createInstance(&vkGPU, 100) != VK_SUCCESS) return;
		if (findPhysicalDevice(&vkGPU) != VK_SUCCESS) return;
		vkGetPhysicalDeviceProperties(vkGPU.physicalDevice, &vkGPU.physicalDeviceProperties);
		vkGetPhysicalDeviceMemoryProperties(vkGPU.physicalDevice, &vkGPU.physicalDeviceMemoryProperties);
		if (getComputeQueueFamilyIndex(&vkGPU) != VK_SUCCESS) return;
		if (createDevice(&vkGPU, 100) != VK_SUCCESS) return;
		if (createFence(&vkGPU) != VK_SUCCESS) return;
		if (createCommandPool(&vkGPU) != VK_SUCCESS) return;

		std::vector<float> kernelReal(totalSize, 0.0f);
		for (int j = 0; j < paddedH; j++)
			for (int i = 0; i < paddedW; i++) {
				auto dx = float(std::min(i, paddedW - i));
				auto dy = float(std::min(j, paddedH - j));
				kernelReal[j * paddedW + i] = 1.0f / (dx * dx + dy * dy + 1.0f);
			}

		bufferSize = (uint64_t)sizeof(float) * 2 * (paddedW / 2 + 1) * paddedH;
		if (allocateBuffer(&vkGPU, &buffer, &bufferDeviceMemory,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, bufferSize) != VKFFT_SUCCESS) return;

		if (transferDataFromCPU(&vkGPU, kernelReal.data(), &buffer, totalSize * sizeof(float)) != VKFFT_SUCCESS) return;

		VkFFTConfiguration kCfg = {};
		VkFFTApplication kApp = {};
		kCfg.FFTdim = 2; kCfg.size[0] = paddedW; kCfg.size[1] = paddedH;
		kCfg.performR2C = true;
		kCfg.device = &vkGPU.device; kCfg.queue = &vkGPU.queue;
		kCfg.fence = &vkGPU.fence; kCfg.commandPool = &vkGPU.commandPool;
		kCfg.physicalDevice = &vkGPU.physicalDevice; kCfg.isCompilerInitialized = 1;
		kCfg.buffer = &buffer; kCfg.bufferSize = &bufferSize;
		if (initializeVkFFT(&kApp, kCfg) != VKFFT_SUCCESS) return;
		VkFFTLaunchParams kl = {}; kl.inputBuffer = &buffer; kl.outputBuffer = &buffer;
		if (VkFFTAppend(&kApp, -1, &kl) != VKFFT_SUCCESS) return;
		kernelCplx.resize(bufferSize / sizeof(float));
		transferDataToCPU(&vkGPU, kernelCplx.data(), &buffer, bufferSize);
		deleteVkFFT(&kApp);

		VkFFTConfiguration fCfg = kCfg;
		fCfg.performR2C = true;
		if (initializeVkFFT(&appFwd, fCfg) != VKFFT_SUCCESS) return;

		fCfg.performR2C = false;
		if (initializeVkFFT(&appInv, fCfg) != VKFFT_SUCCESS) return;

		available = true;
	}

	void Solve(float *src, float *result, int w, int h)
	{
		if (!available) return;
		std::vector<float> padded(totalSize, 0.0f);
		for (int j = 0; j < h; j++)
			for (int i = 0; i < w; i++)
				padded[(j + h) * paddedW + (i + w)] = src[j * w + i];

		transferDataFromCPU(&vkGPU, padded.data(), &buffer, totalSize * sizeof(float));
		VkFFTLaunchParams lf = {}; lf.inputBuffer = &buffer; lf.outputBuffer = &buffer;
		VkFFTAppend(&appFwd, -1, &lf);

		std::vector<float> cplx(bufferSize / sizeof(float));
		transferDataToCPU(&vkGPU, cplx.data(), &buffer, bufferSize);
		uint64_t N = totalSize;
		for (uint64_t k = 0; k < N; k++) {
			float a = cplx[2*k], b = cplx[2*k+1];
			float c = kernelCplx[2*k], d = kernelCplx[2*k+1];
			cplx[2*k]   = (a * c - b * d) / float(N);
			cplx[2*k+1] = (a * d + b * c) / float(N);
		}
		transferDataFromCPU(&vkGPU, cplx.data(), &buffer, bufferSize);

		VkFFTLaunchParams li = {}; li.inputBuffer = &buffer; li.outputBuffer = &buffer;
		VkFFTAppend(&appInv, -1, &li);

		std::vector<float> outReal(totalSize);
		transferDataToCPU(&vkGPU, outReal.data(), &buffer, totalSize * sizeof(float));
		for (int j = 0; j < h; j++)
			for (int i = 0; i < w; i++)
				result[j * w + i] = outReal[(j + h) * paddedW + (i + w)];
	}
};
#endif
