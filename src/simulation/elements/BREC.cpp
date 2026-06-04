#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"
#include "simulation/MagnetismCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_BREC()
{
	Identifier = "DEFAULT_PT_BREC";
	Name = "BREL";
	Colour = 0x707060_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.18f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 2;
	Hardness = 2;

	Weight = 90;

	HeatConduct = 211;
	Description = "Broken electronics. Formed from EMP blasts, and when constantly sparked while under pressure, turns to EXOT.";

	Properties = TYPE_PART|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life)
	{
		if (sim->pv[y/CELL][x/CELL]>10.0f)
		{
			if (parts[i].temp>9000 && sim->pv[y/CELL][x/CELL]>30.0f && sim->rng.chance(1, 200))
			{
				//@ BREC -> EXOT
				sim->part_change_type(i, x, y, PT_EXOT);
				parts[i].life = 1000;
			}
			parts[i].temp += (sim->pv[y/CELL][x/CELL])/8;
		}

	}
	int cx = x/CELL, cy = y/CELL;
	// Induction: shared function handles cooldown, tmp3 tag, Biot-Savart skip
	magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_BREC, 0.5f, 8);
	// Ferromagnetic attraction: pulled toward stronger |B|
	if (sim->magnetismEnabled && cx > 0 && cy > 0 && cx < XCELLS - 1 && cy < YCELLS - 1)
	{
		float dAbsBx = fabsf(sim->bField[cy][cx + 1]) - fabsf(sim->bField[cy][cx - 1]);
		float dAbsBy = fabsf(sim->bField[cy + 1][cx]) - fabsf(sim->bField[cy - 1][cx]);
		float massFactor = 1.0f / (SimulationData::CRef().elements[parts[i].type].Gravity + 0.05f);
		parts[i].vx += dAbsBx * 0.5f * massFactor;
		parts[i].vy += dAbsBy * 0.5f * massFactor;
	}
	electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	// Electromagnetic Lorentz force: F = q(v x B), rotates velocity, preserves |v|
	if (sim->electricityEnabled && sim->magnetismEnabled && parts[i].tmp4 != 0)
	{
		int cx = x/CELL, cy = y/CELL;
		if (cx>=0 && cy>=0 && cx<XCELLS && cy<YCELLS)
		{
			float Bz = sim->bField[cy][cx];
			if (Bz != 0.0f)
			{
				float massFactor = 1.0f / (SimulationData::CRef().elements[parts[i].type].Gravity + 0.05f);
				float dtheta = Bz * parts[i].tmp4 * 0.05f * massFactor;
				float c = cosf(dtheta), s = sinf(dtheta);
				float vx = parts[i].vx * c - parts[i].vy * s;
				float vy = parts[i].vx * s + parts[i].vy * c;
				parts[i].vx = vx; parts[i].vy = vy;
			}
		}
	}
	return 0;
}
