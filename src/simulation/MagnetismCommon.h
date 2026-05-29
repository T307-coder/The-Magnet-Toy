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

// Check if there's an active SPRK anywhere in the same connected conductor block.
// Uses bounded BFS through same-ctype PROP_CONDUCTS neighbors.
// Prevents multiple induction sparks on the same contiguous piece of metal.
static inline bool magnetism_hasNearbySPRK(Simulation *sim, int x, int y, int maxRange, int selfType)
{
	// Bounded BFS: queue of positions to check, visited set
	constexpr int MAX_QUEUE = 80;
	int qx[MAX_QUEUE], qy[MAX_QUEUE];
	bool visited[16][16] = {}; // local grid centered on (x,y), offset +8
	int head = 0, tail = 0;

	int ox = 8 - (x / CELL);
	int oy = 8 - (y / CELL);

	qx[tail] = x / CELL;
	qy[tail] = y / CELL;
	tail++;
	visited[qy[0] + oy][qx[0] + ox] = true;

	while (head < tail && tail < MAX_QUEUE)
	{
		int cx = qx[head], cy = qy[head];
		head++;

		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
			{
				if (!rx && !ry) continue;
				int nx = cx + rx, ny = cy + ry;
				if (nx < 0 || ny < 0 || nx >= XCELLS || ny >= YCELLS) continue;
				int mi = nx + ox, mj = ny + oy;
				if (mi < 0 || mj < 0 || mi >= 16 || mj >= 16) continue;
				if (visited[mj][mi]) continue;

				auto r = sim->pmap[ny][nx];
				if (!r) continue;
				int rt = TYP(r);

				// Check if this neighbor has active SPRK
				if (rt == PT_SPRK && sim->parts[ID(r)].life > 0)
					return true;

				// Follow same-type conductors
				if (rt == selfType && (SimulationData::CRef().elements[rt].Properties & PROP_CONDUCTS))
				{
					visited[mj][mi] = true;
					qx[tail] = nx;
					qy[tail] = ny;
					tail++;
				}
			}
	}
	return false;
}

// Unified magnetic induction: if dB/dt exceeds threshold, convert to SPRK.
// Returns true if SPRK was created (caller should return 1 immediately).
// ctype = element type to set as SPRK's ctype.
// tmp2Ref = reference to this particle's tmp2 (B-field history + cooldown).
// threshold = minimum |dB/dt| to trigger.
// chanceDenom = sim->rng.chance(1, chanceDenom).
// coolFrames = frames of post-induction immunity (prevents re-ignition loops).
static inline bool magnetism_tryInduction(Simulation *sim, int i, int x, int y, int cx, int cy, int &tmp2Ref, int ctype, float threshold, int chanceDenom, int coolFrames)
{
	if (!sim->magnetismEnabled || cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS)
		return false;

	// Cooldown: tmp2Ref negative means recently induced, counting up to 0
	if (tmp2Ref < 0)
	{
		tmp2Ref++;
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
		if (magnetism_hasNearbySPRK(sim, x, y, 0, ctype))
			return false;

		sim->part_change_type(i, x, y, PT_SPRK);
		sim->parts[i].ctype = ctype;
		sim->parts[i].life = 4;
		tmp2Ref = -coolFrames; // start post-induction cooldown
		return true;
	}
	return false;
}
