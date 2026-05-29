#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_EBFM()
{
	Identifier = "DEFAULT_PT_EBFM";
	Name = "EBFM";
	Colour = 0x44CCCC_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 100;

	DefaultProperties.tmp = 5;
	HeatConduct = 251;
	Description = "Electric uniform B-field magnet. Temperature=strength(±273.15=zero), tmp=range. Needs SPRK.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

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
	// Only active when sparked (life > 0)
	if (parts[i].life <= 0) return 0;
	int range = parts[i].tmp;
	if (range <= 0) return 0;
	// Temperature determines strength: 0 at 273.15K, positive above, negative below
	int strength = (int)((parts[i].temp - 273.15f) * 0.1f);
	if (strength == 0) return 0;
	int cx0 = x / CELL, cy0 = y / CELL;
	for (int dy = -range; dy <= range; dy++)
	{
		for (int dx = -range; dx <= range; dx++)
		{
			int cx = cx0 + dx, cy = cy0 + dy;
			if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
				sim->magSrc[cy][cx] += (float)strength;
		}
	}
	return 0;
}
