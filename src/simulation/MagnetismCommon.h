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
	if (!sim->magnetismEnabled || !sim->inductionEnabled || cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS)
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
	if (dBdt > threshold && sim->sharedRng.chance(1, chanceDenom))
	{
		sim->part_change_type_outer(i, x, y, PT_SPRK);
		sim->parts[i].ctype = ctype;
		sim->parts[i].life = 4;
		sim->parts[i].tmp3 = 1;  // mark as induced SPRK → element gets life=100 on death
		return true;
	}
	return false;
}

// Build cached list of magnetic source positions (MAGN, active ELMG, MGPN).
// Call once per frame in BeforeSim. Thread-safe: only writes during setup phase.
static inline void magnetism_buildSourceList(Simulation *sim)
{
	sim->magSourceCount = 0;
	constexpr int maxSources = RenderableSimulation::MAX_MAG_SOURCES;
	auto &parts = sim->parts;
	for (int i = 0; i < parts.active && sim->magSourceCount < maxSources; i++)
	{
		if (!parts[i].type) continue;
		int t = parts[i].type;
		int x = (int)(parts[i].x + 0.5f);
		int y = (int)(parts[i].y + 0.5f);
		int target = 0;
		if (t == PT_MAGN)
		{
			target = parts[i].tmp;
		}
		else if (t == PT_ELMG && parts[i].life == 10)
		{
			target = (int)((parts[i].temp - 273.15f) / 5.0f);
			if (target > 100) target = 100;
			if (target < -100) target = -100;
		}
		else if (t == PT_MGPN)
		{
			target = parts[i].tmp;
		}
		else continue;
		if (target == 0) continue;
		int idx = sim->magSourceCount++;
		sim->magSourceX[idx] = x;
		sim->magSourceY[idx] = y;
		sim->magSourceTarget[idx] = target;
	}
}

// Range-based magnetization: ferromagnets receive magnetization from nearby MAGN/ELMG/MGPN.
// Uses cached source list (push model) to avoid O(N×R²) pmap scanning.
// Source list is rebuilt once per frame in magnetism_buildSourceList.
static inline void magnetism_contactCharge(Simulation *sim, Particle &p, int x, int y, int &tmp3Ref)
{
	int cx = x / CELL, cy = y / CELL;
	if (!sim->magnetismEnabled || cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS) return;
	if (p.temp >= 773.15f) return;

	// Iterate magnetic sources (cached, not full grid scan)
	// Sources are stored as flat arrays in sim: magSourceX/Y[], magSourceTarget[], magSourceCount
	// Rebuilt each frame in BeforeSim via magnetism_buildSourceList.
	// Type: 0=MAGN, 1=ELMG, 2=MGPN
	for (int si = 0; si < sim->magSourceCount; si++)
	{
		int sx = sim->magSourceX[si], sy = sim->magSourceY[si];
		int dist = std::max(abs(x - sx), abs(y - sy));
		int reach = 1 + std::abs(sim->magSourceTarget[si]) / 25;
		if (dist <= reach)
		{
			int target = sim->magSourceTarget[si];
			int chance = dist * dist;
			if (dist <= 1 || sim->sharedRng.chance(1, chance))
			{
				if (tmp3Ref < target) tmp3Ref++;
				else if (tmp3Ref > target) tmp3Ref--;
			}
		}
	}
}
// DEUT-style magnetization diffusion between ferromagnets in range.
// reach = 1 + |strength|/25; stronger magnets spread further.
// Probe count scales with reach² to maintain hit rate.
static inline void magnetism_diffuseCharge(Simulation *sim, Particle &p, int x, int y, int &tmp3Ref)
{
	int reach = 1 + std::abs(tmp3Ref) / 25;
	if (reach < 2) reach = 2;
	if (reach > 15) reach = 15;
	int numTrades = reach * reach / 2; // scale probes with area
	if (numTrades < 4) numTrades = 4;
	for (int trade = 0; trade < numTrades; trade++)
	{
		int rx = sim->sharedRng.between(-reach, reach);
		int ry = sim->sharedRng.between(-reach, reach);
		if (!rx && !ry) continue;
		int nx = x + rx, ny = y + ry;
		if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES) continue;
		auto r = sim->pmap[ny][nx];
		if (!r) continue;
		int rt = TYP(r);
		if (rt == PT_IRON || rt == PT_TTAN || rt == PT_BMTL || rt == PT_BRMT)
		{
			int &other = sim->parts[ID(r)].tmp3;
			int diff = tmp3Ref - other;
			if (diff > 1)
			{
				int transfer = diff / 2;
				other += transfer;
				tmp3Ref -= transfer;
			}
			else if (diff == 1)
			{
				other++;
				tmp3Ref--;
			}
		}
	}
}

// Shared Biot-Savart: add magnetic field contribution from a current element to magSrc.
static inline void magnetism_addBiotSavart(Simulation *sim, float px, float py, float vx, float vy, float scale, int radius)
{
	int pcx = (int)(px) / CELL;
	int pcy = (int)(py) / CELL;
	for (int dy = -radius; dy <= radius; dy++)
		for (int dx = -radius; dx <= radius; dx++)
		{
			int cx = pcx + dx, cy = pcy + dy;
			if (cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS) continue;
			float rx = cx * CELL + CELL * 0.5f - px;
			float ry = cy * CELL + CELL * 0.5f - py;
			float r2 = rx * rx + ry * ry + 1.0f;
			float r = sqrtf(r2);
			float dB = scale * (vx * ry - vy * rx) / (r2 * r);
			sim->magSrc[cy][cx] += dB;
		}
}
