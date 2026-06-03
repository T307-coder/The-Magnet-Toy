#pragma once
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"

// ============================================================================
// Shared electric field interaction functions
// Use these in element update() to avoid duplicating code across 20+ elements
// ============================================================================

// Contact charging: POSC, FIXC, ELEC, PROT transfer charge to this particle.
// chargeRef = reference to the charge variable (tmp4 for most, tmp3 for LITH).
// Clamps charge to [-100, 100] and contributes to eSrc.
static inline void electricity_chargeContact(Simulation *sim, Particle &p, int x, int y, int &chargeRef)
{
	if (!sim->electricityEnabled)
		return;
	int cx = x / CELL, cy = y / CELL;
	if (cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS)
		return;

	for (int rx = -1; rx <= 1; rx++)
		for (int ry = -1; ry <= 1; ry++)
		{
			if (!rx && !ry) continue;
			auto r = sim->pmap[y + ry][x + rx];
			if (r)
			{
				int rt = TYP(r);
				if (rt == PT_POSC && sim->parts[ID(r)].life == 10)
				{
					int q = (int)((sim->parts[ID(r)].temp - 273.15f) / 5.0f);
					if (q > 100) q = 100; if (q < -100) q = -100;
					if (chargeRef < q) chargeRef++; else if (chargeRef > q) chargeRef--;
				}
				else if (rt == PT_FIXC)
				{
					int q = sim->parts[ID(r)].tmp;
					if (q > 100) q = 100; if (q < -100) q = -100;
					if (chargeRef < q) chargeRef++; else if (chargeRef > q) chargeRef--;
				}
			}
			auto pr = sim->photons[y + ry][x + rx];
			if (pr)
			{
				int prt = TYP(pr);
				if (prt == PT_ELEC) { if (chargeRef > -100) chargeRef--; }
				else if (prt == PT_PROT) { if (chargeRef < 100) chargeRef++; }
			}
		}

	if (chargeRef > 100) chargeRef = 100;
	if (chargeRef < -100) chargeRef = -100;
	if (chargeRef != 0)
		sim->eSrc[cy][cx] += chargeRef * 0.05f;
}

// Charge diffusion: DEUT-style random trade between PROP_CONDUCTS neighbors.
// chargeRef = reference to charge variable for THIS particle.
// For neighbors, automatically uses tmp4 (or tmp3 for LITH).
static inline void electricity_diffuseCharge(Simulation *sim, Particle &p, int x, int y, int &chargeRef)
{
	for (int trade = 0; trade < 4; trade++)
	{
		int rx = sim->rng.between(-2, 2);
		int ry = sim->rng.between(-2, 2);
		if (!rx && !ry) continue;
		auto r = sim->pmap[y + ry][x + rx];
		if (!r) continue;
		if (!(SimulationData::CRef().elements[TYP(r)].Properties & PROP_CONDUCTS))
			continue;

		int &neighborCharge = (TYP(r) == PT_LITH)
			? sim->parts[ID(r)].tmp3
			: sim->parts[ID(r)].tmp4;

		int diff = chargeRef - neighborCharge;
		if (diff > 1) { int t = diff / 2; neighborCharge += t; chargeRef -= t; }
		else if (diff == 1) { neighborCharge++; chargeRef--; }
	}

	// Dielectric polarization: directional transfer along E-field gradient.
	// Rate-limited: slower than diffusion so conductors can depolarize naturally.
	if (sim->polarizationEnabled && sim->rng.chance(1, 5))
	{
		int cx = x / CELL, cy = y / CELL;
		if (cx > 0 && cy > 0 && cx < XCELLS - 1 && cy < YCELLS - 1)
		{
			float dEx = sim->eField[cy][cx + 1] - sim->eField[cy][cx - 1];
			float dEy = sim->eField[cy + 1][cx] - sim->eField[cy - 1][cx];
			float Emag = std::sqrt(dEx * dEx + dEy * dEy);
			if (Emag >= 3.0f)
			{
				int chargeLimit = (int)(Emag * 2.0f);
				if (chargeLimit > 100) chargeLimit = 100;
				if (chargeLimit < 3) chargeLimit = 3;
				if (std::abs(chargeRef) < chargeLimit)
				{
					int dx = (dEx > 1.0f) ? 1 : (dEx < -1.0f) ? -1 : 0;
					int dy = (dEy > 1.0f) ? 1 : (dEy < -1.0f) ? -1 : 0;
					if (dx || dy)
					{
						auto r = sim->pmap[y + dy][x + dx];
						if (r && (SimulationData::CRef().elements[TYP(r)].Properties & PROP_CONDUCTS))
						{
							int &nbr = (TYP(r) == PT_LITH) ? sim->parts[ID(r)].tmp3 : sim->parts[ID(r)].tmp4;
							if (std::abs(nbr) < chargeLimit)
							{
								// Only polarize toward equilibrium, not past it.
								// Gradient+ side: this particle should become +, neighbor -
								if ((dx > 0 || dy > 0) && chargeRef <= 0 && nbr >= 0)
									{ chargeRef++; nbr--; }
								else if ((dx < 0 || dy < 0) && chargeRef >= 0 && nbr <= 0)
									{ chargeRef--; nbr++; }
							}
						}
					}
				}
			}
		}
	}
}

// Dielectric polarization: directional charge transfer along E-field gradient.
// Unlike diffusion (random averaging), this creates opposite charges on opposite
// sides of a conductor, like real dielectric polarization in an external field.
// "I give 1, you take 1" — discrete transfer, not averaging.
// Saturation: |charge| capped by local |E| * 5 (polarization can't exceed field).
static inline void electricity_polarizeCharge(Simulation *sim, Particle &p, int x, int y, int &chargeRef)
{
	if (!sim->electricityEnabled || !sim->polarizationEnabled) return;

	int cx = x / CELL, cy = y / CELL;
	if (cx <= 0 || cy <= 0 || cx >= XCELLS - 1 || cy >= YCELLS - 1) return;

	// Local E-field gradient
	float dEx = sim->eField[cy][cx + 1] - sim->eField[cy][cx - 1];
	float dEy = sim->eField[cy + 1][cx] - sim->eField[cy - 1][cx];
	float Emag = std::sqrt(dEx * dEx + dEy * dEy);
	if (Emag < 1.0f) return;

	// Saturation limit proportional to field strength
	int chargeLimit = (int)(Emag * 5.0f);
	if (chargeLimit > 100) chargeLimit = 100;
	if (chargeLimit < 5) chargeLimit = 5;
	if (std::abs(chargeRef) >= chargeLimit) return;

	// Direction: find neighbor in field gradient direction
	int dx = (dEx > 1.0f) ? 1 : (dEx < -1.0f) ? -1 : 0;
	int dy = (dEy > 1.0f) ? 1 : (dEy < -1.0f) ? -1 : 0;
	if (!dx && !dy) return;

	auto r = sim->pmap[y + dy][x + dx];
	if (!r) return;
	auto &sd = SimulationData::CRef();
	if (!(sd.elements[TYP(r)].Properties & PROP_CONDUCTS)) return;

	int &nbrCharge = (TYP(r) == PT_LITH) ? sim->parts[ID(r)].tmp3 : sim->parts[ID(r)].tmp4;
	int nbrLimit = (int)(Emag * 5.0f);
	if (nbrLimit > 100) nbrLimit = 100;
	if (nbrLimit < 5) nbrLimit = 5;
	if (std::abs(nbrCharge) >= nbrLimit) return;

	// Transfer 1 charge unit in direction of field gradient
	// Positive gradient direction → this particle becomes more positive, neighbor more negative
	if (dx > 0 || dy > 0) { chargeRef++; nbrCharge--; }
	else { chargeRef--; nbrCharge++; }
}

// Electric force: Coulomb (charged) or dielectrophoresis (uncharged).
// charge = current charge value (tmp4 or tmp3). 0 = DEP only, !=0 = Coulomb only.
// baselineDEP = DEP coefficient for uncharged state (0.5 for powders, 1.0 for liquid metals).
static inline void electricity_applyForce(Simulation *sim, Particle &p, int x, int y, int charge, float baselineDEP)
{
	if (!sim->electricityEnabled)
		return;
	int cx = x / CELL, cy = y / CELL;
	if (cx <= 0 || cy <= 0 || cx >= XCELLS - 1 || cy >= YCELLS - 1)
		return;

	float dEx = sim->eField[cy][cx + 1] - sim->eField[cy][cx - 1];
	float dEy = sim->eField[cy + 1][cx] - sim->eField[cy - 1][cx];
	float massFactor = 1.0f / (SimulationData::CRef().elements[p.type].Gravity + 0.05f);

	if (charge != 0)
	{
		// Coulomb: F = -q * grad(V), ELEC/PROT standard coefficient 0.5
		p.vx -= dEx * charge * 0.5f * massFactor;
		p.vy -= dEy * charge * 0.5f * massFactor;
	}
	else
	{
		// Dielectrophoresis: neutral conductor attracted toward stronger |E|
		float dAbsEx = fabsf(sim->eField[cy][cx + 1]) - fabsf(sim->eField[cy][cx - 1]);
		float dAbsEy = fabsf(sim->eField[cy + 1][cx]) - fabsf(sim->eField[cy - 1][cx]);
		p.vx += dAbsEx * baselineDEP * massFactor;
		p.vy += dAbsEy * baselineDEP * massFactor;
	}
}

// Lorentz force: charged particle rotates in B-field. Preserves |v| (energy-conserving).
// charge = current charge value.
static inline void electricity_applyLorentz(Simulation *sim, Particle &p, int x, int y, int charge)
{
	if (!sim->electricityEnabled || !sim->magnetismEnabled || charge == 0)
		return;
	int cx = x / CELL, cy = y / CELL;
	if (cx < 0 || cy < 0 || cx >= XCELLS || cy >= YCELLS)
		return;

	float Bz = sim->bField[cy][cx];
	if (Bz == 0.0f) return;

	float massFactor = 1.0f / (SimulationData::CRef().elements[p.type].Gravity + 0.05f);
	float dtheta = Bz * charge * 0.05f * massFactor;
	float c = cosf(dtheta), s = sinf(dtheta);
	float vx = p.vx * c - p.vy * s;
	float vy = p.vx * s + p.vy * c;
	p.vx = vx; p.vy = vy;
}
