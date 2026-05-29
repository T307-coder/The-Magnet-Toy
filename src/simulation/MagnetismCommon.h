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

// Unified magnetic induction: if dB/dt exceeds threshold, convert to SPRK.
// Returns true if SPRK was created (caller should return 1 immediately).
// ctype = element type to set as SPRK's ctype.
// tmp2Ref = reference to this particle's tmp2 (B-field history, encoded as int(B*10000)).
// threshold = minimum |dB/dt| to trigger.
// chanceDenom = sim->rng.chance(1, chanceDenom).
// Cooldown: element life acts as cooldown timer (set to 100 when induced SPRK dies).
static inline bool magnetism_tryInduction(Simulation *sim, int i, int x, int y, int cx, int cy, int &tmp2Ref, int ctype, float threshold, int chanceDenom)
{
	if (!sim->magnetismEnabled || cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS)
		return false;

	// Cooldown: element life > 0 means recently induced (PROP_LIFE_DEC counts it down)
	// Still track B-field to avoid stale dB/dt when cooldown ends
	if (sim->parts[i].life > 0)
	{
		tmp2Ref = (int)(sim->bField[cy][cx] * 10000.0f);
		return false;
	}

	float Bnow = sim->bField[cy][cx];
	float Bprev = (tmp2Ref == 0) ? Bnow : tmp2Ref / 10000.0f;
	tmp2Ref = (int)(Bnow * 10000.0f);

	if (!sim->prevBFieldValid)
		return false;

	float dBdt = fabsf(Bnow - Bprev);
	if (dBdt > threshold && sim->rng.chance(1, chanceDenom))
	{
		sim->part_change_type(i, x, y, PT_SPRK);
		sim->parts[i].ctype = ctype;
		sim->parts[i].life = 4;
		sim->parts[i].tmp3 = 1;  // mark as induced SPRK → element gets life=100 on death
		return true;
	}
	return false;
}
