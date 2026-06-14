#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_EBFM()
{
	Identifier = "DEFAULT_PT_EBFM";
	Name = "EBFM";
	Colour = 0x0A7B5B_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWERED;
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
	HeatConduct = 0;
	Description = Localization::Ref().Tr("sim.elem.DEFAULT_PT_EBFM");

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	DefaultProperties.life = 10;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life != 10)
	{
		if (parts[i].life > 0)
			parts[i].life--;
	}
	else
	{
		int range = parts[i].tmp;
		if (range <= 0) return 0;
		int strength = (int)((parts[i].temp - 273.15f) * 0.1f);
		if (strength == 0) return 0;
		int cx0 = x / CELL, cy0 = y / CELL;
		int r2 = range * range;
		for (int dy = -range; dy <= range; dy++)
		{
			for (int dx = -range; dx <= range; dx++)
			{
				if (dx*dx + dy*dy > r2) continue;
				int cx = cx0 + dx, cy = cy0 + dy;
				if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
					sim->bField[cy][cx] += (float)strength;
			}
		}
		// Propagate state to neighbors (like GPMP/ELMG)
		for (auto rx = -2; rx <= 2; rx++)
		{
			for (auto ry = -2; ry <= 2; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (!r) continue;
					if (TYP(r) == PT_EBFM)
					{
						if (parts[ID(r)].life < 10 && parts[ID(r)].life > 0)
							parts[i].life = 9;
						else if (parts[ID(r)].life == 0)
							parts[ID(r)].life = 10;
					}
				}
			}
		}
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life == 10)
	{
		*colr = 30; *colg = 180; *colb = 140;
	}
	else
	{
		*colr = 10; *colg = 60; *colb = 50;
	}
	return 0;
}
