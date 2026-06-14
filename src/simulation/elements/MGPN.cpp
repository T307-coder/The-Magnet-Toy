#include "simulation/ElementCommon.h"
#include <cmath>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_MGPN()
{
	Identifier = "DEFAULT_PT_MGPN";
	Name = "MGPN";
	Colour = 0xAA44AA_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = -.99f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = -1;

	DefaultProperties.tmp = 7;
	HeatConduct = 61;
	Description = Localization::Ref().Tr("sim.elem.DEFAULT_PT_MGPN");

	Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

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
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = sim->rng.between(0, 359) * std::numbers::pi_v<float> / 180.0f;
	sim->parts[i].life = 250 + sim->rng.between(0, 199);
	sim->parts[i].vx = 2.0f * cosf(a);
	sim->parts[i].vy = 2.0f * sinf(a);
}

static int update(UPDATE_FUNC_ARGS)
{
	// Clamp tmp (like GRVT)
	if (parts[i].tmp >= 100)
		parts[i].tmp = 100;
	if (parts[i].tmp <= -100)
		parts[i].tmp = -100;

	// Write to magnetic source field (like GRVT writes to gravIn.mass)
	int cx = x / CELL, cy = y / CELL;
	if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
		sim->magSrc[cy][cx] = 0.2f * parts[i].tmp;

	// Force on monopole: F = q_m * ∇B (field gradient, like GRVT in gravity field)
	if (sim->magnetismEnabled && cx > 0 && cy > 0 && cx < XCELLS - 1 && cy < YCELLS - 1)
	{
		float dBx = sim->bField[cy][cx + 1] - sim->bField[cy][cx - 1];
		float dBy = sim->bField[cy + 1][cx] - sim->bField[cy - 1][cx];
		float qm = parts[i].tmp * 0.3f;
		parts[i].vx += dBx * qm;
		parts[i].vy += dBy * qm;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Purple-purple glow
	*colr = 170;
	*colg = 68;
	*colb = 170;
	return 0;
}
