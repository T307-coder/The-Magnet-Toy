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

// Check if there's an active SPRK nearby (within induction propagation range).
// Prevents feedback loops when dB/dt triggers multiple sparks on connected metal.
static inline bool magnetism_hasNearbySPRK(Simulation *sim, int x, int y, int range)
{
	for (int rx = -range; rx <= range; rx++)
		for (int ry = -range; ry <= range; ry++)
		{
			if (std::abs(rx) + std::abs(ry) > range) continue;
			auto r = sim->pmap[y + ry][x + rx];
			if (r && TYP(r) == PT_SPRK && sim->parts[ID(r)].life > 0)
				return true;
		}
	return false;
}

// Unified magnetic induction: if dB/dt exceeds threshold, convert to SPRK.
// Returns true if SPRK was created (caller should return 1 immediately).
// ctype = element type to set as SPRK's ctype.
// tmp2Ref = reference to this particle's tmp2 (B-field history storage).
// threshold = minimum |dB/dt| to trigger.
// chanceDenom = sim->rng.chance(1, chanceDenom).
// sprkRange = max Manhattan distance to check for existing SPRK (prevents feedback).
static inline bool magnetism_tryInduction(Simulation *sim, int i, int x, int y, int cx, int cy, int &tmp2Ref, int ctype, float threshold, int chanceDenom, int sprkRange)
{
	if (!sim->magnetismEnabled || cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS)
		return false;

	float Bnow = sim->bField[cy][cx];
	float Bprev = (tmp2Ref == 0) ? Bnow : tmp2Ref / 10000.0f;
	tmp2Ref = (int)(Bnow * 10000.0f);

	if (!sim->prevBFieldValid)
		return false;

	float dBdt = fabsf(Bnow - Bprev);
	if (dBdt > threshold && sim->rng.chance(1, chanceDenom))
	{
		if (magnetism_hasNearbySPRK(sim, x, y, sprkRange))
			return false;

		sim->part_change_type(i, x, y, PT_SPRK);
		sim->parts[i].ctype = ctype;
		sim->parts[i].life = 4;
		return true;
	}
	return false;
}
