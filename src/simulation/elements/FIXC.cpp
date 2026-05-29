#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_FIXC()
{
	Identifier = "DEFAULT_PT_FIXC";
	Name = "FIXC";
	Colour = 0xFFDD44_rgb;
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
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 100;

	DefaultProperties.tmp = 1;
	HeatConduct = 0;
	Description = "Fixed charge. tmp>0=positive(yellow), tmp<0=negative(cyan), |tmp|=strength.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	ASSIGN_SIM_CALLBACK(Update, update)
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int cx = x / CELL;
	int cy = y / CELL;
	if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
		sim->eSrc[cy][cx] = parts[i].tmp * 1.0f;
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int str = std::abs(cpart->tmp);
	if (str > 0)
	{
		if (cpart->tmp > 0) { *colr = 255; *colg = 220 - str * 10; *colb = 60; }
		else                { *colr = 60; *colg = 220 - str * 10; *colb = 255; }
	}
	return 0;
}
