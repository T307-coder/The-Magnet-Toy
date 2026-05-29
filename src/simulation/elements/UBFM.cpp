#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_UBFM()
{
	Identifier = "DEFAULT_PT_UBFM";
	Name = "UBFM";
	Colour = 0xCC44CC_rgb;
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

	DefaultProperties.tmp = 10;
	DefaultProperties.tmp2 = 5;
	HeatConduct = 0;
	Description = "Uniform B-field magnet. tmp=N/S strength(±), tmp2=range(cells).";

	Properties = TYPE_SOLID;

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
	// Add uniform magnetic source to all cells within tmp2 range
	int strength = parts[i].tmp;
	int range = parts[i].tmp2;
	if (strength == 0 || range <= 0) return 0;
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
