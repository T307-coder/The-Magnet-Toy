#include "simulation/ElementCommon.h"
#include "simulation/MagnetismCommon.h"
#include "simulation/ElectricityCommon.h"
#include "simulation/Air.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_TTAN()
{
	Identifier = "DEFAULT_PT_TTAN";
	Name = "TTAN";
	Colour = 0x909090_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 48;

	Weight = 100;

	HeatConduct = 251;
	Description = "Titanium. Higher melting temperature than most other metals, blocks all air pressure.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_HOT_GLOW|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1941.0f;
	HighTemperatureTransition = PT_LAVA; //@ TTAN -> LAVA(TTAN)

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	int ttan = 0;
	if (nt <= 2)
		ttan = 2;
	else if (parts[i].tmp)
		ttan = 2;
	else if (nt <= 6)
	{
		for (int rx = -1; rx <= 1; rx++)
		{
			for (int ry = -1; ry <= 1; ry++)
			{
				if (!rx != !ry)
				{
					if (TYP(pmap[y+ry][x+rx]) == PT_TTAN)
						ttan++;
				}
			}
		}
	}

	if (ttan >= 2)
	{
		sim->air->bmap_blockair[y/CELL][x/CELL] = 1;
		sim->air->bmap_blockairh[y/CELL][x/CELL] = 0x8;
	}
	// Magnetization: contact with magnets + DEUT-style internal diffusion
	int cx = x/CELL, cy = y/CELL;
	if (sim->magnetismEnabled && cx>=0 && cx<XCELLS && cy>=0 && cy<YCELLS)
	{
		if (parts[i].temp < 773.15f)
		{
			magnetism_contactCharge(sim, parts[i], x, y, parts[i].tmp3);
			magnetism_diffuseCharge(sim, parts[i], x, y, parts[i].tmp3);
		}
		else
		{
			if (parts[i].tmp3 > 0) parts[i].tmp3 = std::max(0, parts[i].tmp3 - 5);
			else if (parts[i].tmp3 < 0) parts[i].tmp3 = std::min(0, parts[i].tmp3 + 5);
		}

		if (parts[i].tmp3 != 0)
			sim->magSrc[cy][cx] += parts[i].tmp3 * 0.02f;
	}
	// Induction: only when completely unmagnetized
	if (parts[i].tmp3 == 0 && magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_TTAN, 1.5f, 6))
		return 1;
	if (parts[i].tmp3 != 0) parts[i].life = 100;
	electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	return 0;
}
