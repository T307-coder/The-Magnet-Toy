#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_MAGN()
{
	Identifier = "DEFAULT_PT_MAGN";
	Name = "MAGN";
	Colour = 0xCC2222_rgb;
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

	DefaultProperties.tmp = 1;
	HeatConduct = 0;
	Description = Localization::Ref().Tr("sim.elem.DEFAULT_PT_MAGN");

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
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Like GPMP: tmp controls polarity & strength
	// tmp>0 = N pole (positive B), tmp<0 = S pole (negative B)
	int cx = x / CELL;
	int cy = y / CELL;
	if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
		sim->magSrc[cy][cx] = parts[i].tmp * 1.0f;
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// N pole (tmp>0): red, S pole (tmp<0): blue, intensity = |tmp|
	int str = std::abs(cpart->tmp);
	if (str > 0)
	{
		if (cpart->tmp > 0) { *colr = 200 + str * 10; *colg = 30; *colb = 30; }
		else                { *colr = 30; *colg = 30; *colb = 200 + str * 10; }
	}
	return 0;
}
