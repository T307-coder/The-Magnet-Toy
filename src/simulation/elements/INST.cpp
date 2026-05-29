#include "simulation/ElementCommon.h"
#include "simulation/MagnetismCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_INST()
{
	Identifier = "DEFAULT_PT_INST";
	Name = "INST";
	Colour = 0x404039_rgb;
	MenuVisible = 1;
	MenuSection = SC_ELEC;
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
	Hardness = 1;

	Weight = 100;

	HeatConduct = 251;
	Description = "Instantly conducts, PSCN to charge, NSCN to take.";

	Properties = TYPE_SOLID|PROP_LIFE_DEC;

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
	int cx = x/CELL, cy = y/CELL;
	if (sim->magnetismEnabled && cx>=0 && cx<XCELLS && cy>=0 && cy<YCELLS)
	{
		float Bnow = sim->bField[cy][cx];
		float Bprev = parts[i].tmp2 / 10000.0f;
		parts[i].tmp2 = (int)(Bnow * 10000.0f);
		if (sim->prevBFieldValid)
		{
			float dBdt = fabsf(Bnow - Bprev);
			if (dBdt > 0.5f && sim->rng.chance(1, 2) && !magnetism_hasNearbySPRK(sim, x, y, 2))
			{
				sim->part_change_type(i, x, y, PT_SPRK);
				parts[i].ctype = PT_INST;
				parts[i].life = 4;
				return 1;
			}
		}
	}
	return 0;
}
