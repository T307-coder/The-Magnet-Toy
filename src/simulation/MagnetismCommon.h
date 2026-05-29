#pragma once
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"

// ============================================================================
// Shared magnetic field interaction functions
// ============================================================================

// Ferromagnetic attraction: neutral ferromagnetic pulled toward stronger |B|.
static inline void magnetism_ferromagneticPull(Simulation *sim, Particle &p, int cx, int cy)
{
	if (!sim->magnetismEnabled || cx <= 0 || cy <= 0 || cx >= XCELLS - 1 || cy >= YCELLS - 1)
		return;

	float dAbsBx = fabsf(sim->bField[cy][cx + 1]) - fabsf(sim->bField[cy][cx - 1]);
	float dAbsBy = fabsf(sim->bField[cy + 1][cx]) - fabsf(sim->bField[cy - 1][cx]);
	float massFactor = 1.0f / (SimulationData::CRef().elements[p.type].Gravity + 0.05f);
	p.vx += dAbsBx * 0.5f * massFactor;
	p.vy += dAbsBy * 0.5f * massFactor;
}
