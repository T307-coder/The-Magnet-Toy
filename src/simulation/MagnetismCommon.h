#pragma once
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/ElementClasses.h"

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

// Para/diamagnetic gradient force: weak pull toward (para, scale>0) or push
// from (dia, scale<0) stronger |B|. Call from BeforeSim for non-ferro types.
// Scale: para ~0.02 (O2, LOXY, URAN), dia ~0.005 (water, carbon, salts, etc.)
// Both ~25-100x weaker than ferromagnetic (0.5).
static inline void magnetism_nonferroForce(Simulation *sim, Particle &p, int cx, int cy, float scale)
{
	if (!sim->magnetismEnabled || !sim->nonferroFieldsEnabled) return;
	if (cx <= 0 || cy <= 0 || cx >= XCELLS - 1 || cy >= YCELLS - 1) return;

	float dAbsBx = fabsf(sim->bField[cy][cx + 1]) - fabsf(sim->bField[cy][cx - 1]);
	float dAbsBy = fabsf(sim->bField[cy + 1][cx]) - fabsf(sim->bField[cy - 1][cx]);
	float massFactor = 1.0f / (SimulationData::CRef().elements[p.type].Gravity + 0.05f);
	p.vx += dAbsBx * scale * massFactor;
	p.vy += dAbsBy * scale * massFactor;
}

// Returns true if the given type is paramagnetic (weakly attracted to strong |B|).
static inline bool magnetism_isParamagnetic(int type)
{
	return type == PT_O2 || type == PT_URAN || type == PT_PLUT;
}

// Returns true if the given type is diamagnetic (weakly repelled from strong |B|).
static inline bool magnetism_isDiamagnetic(int type)
{
	return type == PT_WATR || type == PT_SLTW || type == PT_CBNW ||
	       type == PT_SNOW || type == PT_BGLA || type == PT_SALT ||
	       type == PT_SAWD || type == PT_BCOL;
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
	if (dBdt > threshold && sim->rng.chance(1, chanceDenom))
	{
		sim->part_change_type(i, x, y, PT_SPRK);
		sim->parts[i].ctype = ctype;
		sim->parts[i].life = 4;
		sim->parts[i].tmp3 = 1;  // mark as induced SPRK
		return true;
	}
	return false;
}

// Build cached list of magnetic source positions (MAGN, active ELMG, MGPN).
// Call once per frame in BeforeSim. O(N) scan, thread-safe during setup phase.
static inline void magnetism_buildSourceList(Simulation *sim)
{
	sim->magSourceCount = 0;
	constexpr int maxSources = 4096;
	auto &parts = sim->parts;
	for (int i = 0; i < parts.active && sim->magSourceCount < maxSources; i++)
	{
		if (!parts[i].type) continue;
		int t = parts[i].type;
		int x = (int)(parts[i].x + 0.5f);
		int y = (int)(parts[i].y + 0.5f);
		int target = 0;
		if (t == PT_MAGN)
			target = parts[i].tmp;
		else if (t == PT_ELMG && parts[i].life == 10)
		{
			target = (int)((parts[i].temp - 273.15f) / 5.0f);
		}
		else if (t == PT_MGPN)
			target = parts[i].tmp;
		else continue;
		if (target == 0) continue;
		int idx = sim->magSourceCount++;
		sim->magSourceX[idx] = x;
		sim->magSourceY[idx] = y;
		sim->magSourceTarget[idx] = target;
	}

	// Coil probes: SPRK current creates ± probes along normal (right-hand rule)
	if (sim->sprkCurrentEnabled && sim->coilMagnetizeEnabled && sim->magSourceCount < maxSources - 2)
	{
		for (int i = 0; i < parts.active && sim->magSourceCount < maxSources - 2; i++)
		{
			if (parts[i].type != PT_SPRK) continue;
			if (parts[i].life <= 0 || parts[i].tmp3 == 1) continue;
			int rx = parts[i].tmp5, ry = parts[i].tmp6;
			if (!rx && !ry) continue;
			// Normal direction: B +z points along (-ry, rx)
			// Place probes 6 cells from SPRK along normal
			float mag = std::sqrt((float)(rx*rx + ry*ry));
			int nx = (int)(-ry / mag * 6.0f);
			int ny = (int)( rx / mag * 6.0f);
			int sx = (int)(parts[i].x + 0.5f);
			int sy = (int)(parts[i].y + 0.5f);
			int target = 80 + (parts[i].life - 1) * 20; // life 4→140, life 1→80
			// + side (B out of plane): target > 0
			if (sim->magSourceCount < maxSources) {
				sim->magSourceX[sim->magSourceCount] = sx + nx;
				sim->magSourceY[sim->magSourceCount] = sy + ny;
				sim->magSourceTarget[sim->magSourceCount] = target;
				sim->magSourceCount++;
			}
			// - side (B into plane): target < 0
			if (sim->magSourceCount < maxSources) {
				sim->magSourceX[sim->magSourceCount] = sx - nx;
				sim->magSourceY[sim->magSourceCount] = sy - ny;
				sim->magSourceTarget[sim->magSourceCount] = -target;
				sim->magSourceCount++;
			}
		}
	}
}

// Range-based magnetization: ferromagnets receive magnetization from cached source list.
// Push model: O(S × N) vs old scan model O(N × R²), S << N.
static inline void magnetism_contactCharge(Simulation *sim, Particle &p, int x, int y, int &tmp3Ref)
{
	int cx = x / CELL, cy = y / CELL;
	if (!sim->magnetismEnabled || cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS) return;
	if (p.temp >= 773.15f) return;

	for (int si = 0; si < sim->magSourceCount; si++)
	{
		int sx = sim->magSourceX[si], sy = sim->magSourceY[si];
		int dist = std::max(abs(x - sx), abs(y - sy));
		int reach = 1 + std::abs(sim->magSourceTarget[si]) / 25;
		if (dist <= reach)
		{
			int target = sim->magSourceTarget[si];
			int chance = dist * dist;
			if (dist <= 1 || sim->rng.chance(1, chance))
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

	int numTrades = reach * reach / 2; // scale probes with area
	if (numTrades < 4) numTrades = 4;
	for (int trade = 0; trade < numTrades; trade++)
	{
		int rx = sim->rng.between(-reach, reach);
		int ry = sim->rng.between(-reach, reach);
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

// Shared ferromagnet magnetization update: contact, diffusion, decay, magSrc.
// Used by BMTL, BRMT, IRON, TTAN. Returns cx,cy by reference for reuse.
static inline void magnetism_ferromagnetUpdate(Simulation *sim, Particle &p, int x, int y, int &tmp3Ref, int &cx, int &cy)
{
	cx = x / CELL; cy = y / CELL;
	if (!sim->magnetismEnabled || cx < 0 || cx >= XCELLS || cy < 0 || cy >= YCELLS)
		return;

	if (p.temp < 773.15f)
	{
		magnetism_contactCharge(sim, p, x, y, tmp3Ref);
		magnetism_diffuseCharge(sim, p, x, y, tmp3Ref);
	}
	else
	{
		if (tmp3Ref > 0) tmp3Ref = std::max(0, tmp3Ref - 5);
		else if (tmp3Ref < 0) tmp3Ref = std::min(0, tmp3Ref + 5);
	}

	if (tmp3Ref != 0)
	{
		sim->magSrc[cy][cx] += tmp3Ref * 0.02f;
		// Keep element life > 0 while magnetized → blocks old induction (life cooldown)
		p.life = 100;
	}
}

// New EM induction: dB/dt drives charge separation between conductors.
// Electrons drift opposite to induced E: v_e = sign(dB/dt) * (dB/dy, -dB/dx).
// Same pattern as electricity_polarizeCharge — directional transfer, not averaging.
static inline void magnetism_newInduction(Simulation *sim, Particle &p, int x, int y, int &chargeRef)
{
	if (!sim->magnetismEnabled || !sim->electricityEnabled || !sim->newInductionEnabled) return;

	int cx = x / CELL, cy = y / CELL;
	if (cx <= 0 || cy <= 0 || cx >= XCELLS - 1 || cy >= YCELLS - 1) return;
	if (!sim->prevBFieldValid) return;

	// Local B-field change
	float dBdt = sim->bField[cy][cx] - sim->prevBField[cy][cx];
	float dBmag = std::fabs(dBdt);
	if (dBmag < 0.001f) return;

	// Local B-field gradient (4-neighbour central difference)
	float dBdx = sim->bField[cy][cx + 1] - sim->bField[cy][cx - 1];
	float dBdy = sim->bField[cy + 1][cx] - sim->bField[cy - 1][cx];

	// Electron drift: v_e = sign(dBdt) * (dB/dy, -dB/dx)
	// Each axis independent — allows diagonal drift perpendicular to ∇B
	int dx = 0, dy = 0;
	if (std::fabs(dBdy) > 0.001f)
		dx = ((dBdt > 0) == (dBdy > 0)) ? 1 : -1;
	if (std::fabs(dBdx) > 0.001f)
		dy = ((dBdt > 0) == (dBdx > 0)) ? -1 : 1;
	if (!dx && !dy) return;

	auto r = sim->pmap[y + dy][x + dx];
	if (!r) return;
	auto &sd = SimulationData::CRef();
	if (!(sd.elements[TYP(r)].Properties & PROP_CONDUCTS)) return;

	// Water-based conductors are poor electrolytes — no meaningful induced current
	if (TYP(r) == PT_WATR || TYP(r) == PT_SLTW || TYP(r) == PT_CBNW ||
	    TYP(r) == PT_SNOW) return;

	int &nbrCharge = (TYP(r) == PT_LITH) ? sim->parts[ID(r)].tmp3 : sim->parts[ID(r)].tmp4;

	// Transfer proportional to dB/dt: faster change → more charge moved
	int transfer = (int)(dBmag * 50.0f);
	if (transfer < 1) transfer = 1;
	if (transfer > 5) transfer = 5;
	nbrCharge -= transfer;
	chargeRef += transfer;
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

// Coil magnetization: SPRK current magnetizes nearby ferromagnets directionally.
// Uses same radius-based search as Biot-Savart. Sign from right-hand rule:
// B ∝ (v × r)_z = vx*ry - vy*rx → +B side gets +tmp3, -B side gets -tmp3.
static inline void magnetism_coilMagnetize(Simulation *sim, float px, float py, float vx, float vy, float scale, int radius)
{
	if (!sim->magnetismEnabled) return;
	float mag = std::sqrt(vx * vx + vy * vy);
	if (mag < 0.5f) return;
	int pcx = (int)(px) / CELL;
	int pcy = (int)(py) / CELL;
	int ms = (int)(scale * 5.0f);
	if (ms < 1) ms = 1;
	if (ms > 80) ms = 80;

	for (int dy = -radius; dy <= radius; dy++)
	{
		for (int dx = -radius; dx <= radius; dx++)
		{
			int cx = pcx + dx, cy = pcy + dy;
			if (cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS) continue;
			int p = sim->pmap[cy][cx];
			if (!p) continue;
			int t = TYP(p);
			if (!(t == PT_IRON || t == PT_BMTL || t == PT_BRMT || t == PT_TTAN)) continue;

			auto &part = sim->parts[ID(p)];
			if (part.temp >= 773.15f) continue;

			// Biot-Savart cross product: v × r
			float rx = cx * CELL + CELL * 0.5f - px;
			float ry = cy * CELL + CELL * 0.5f - py;
			float cross = vx * ry - vy * rx;

			int &m = part.tmp3;
			if (cross > 0) { m += ms; if (m > 100) m = 100; }
			else           { m -= ms; if (m < -100) m = -100; }
		}
	}
}
