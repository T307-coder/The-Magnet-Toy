#pragma once
// VkFFT-based FFT Poisson solver — cross-vendor GPU (NVIDIA/AMD/Intel)
// Requires: Vulkan SDK 1.3+, VkFFT header (subprojects/vkfft/vkFFT/vkFFT.h)
// Compile with meson -Duse_vkfft=true
// NOTE: WIP — needs VkFFT API alignment. Currently stubbed out.

#include "SimulationConfig.h"

#ifdef USE_VKFFT
struct VkFFTSolver
{
	bool available = false;

	void Init(int, int) { available = false; }
	void Solve(float *, float *, int, int) {}
	void Release() {}
};
#endif
