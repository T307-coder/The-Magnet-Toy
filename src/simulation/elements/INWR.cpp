#include "simulation/ElementCommon.h"
#include "simulation/MagnetismCommon.h"
#include "simulation/ElectricityCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_INWR()
{
	Identifier = "DEFAULT_PT_INWR";
	Name = "INWR";
	Colour = 0x544141_rgb;
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
	Description = Localization::Ref().Tr("sim.elem.DEFAULT_PT_INWR");

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1687.0f;
	HighTemperatureTransition = PT_LAVA; //@ INWR -> LAVA(INWR)

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	int cx = x/CELL, cy = y/CELL;
	if (magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_INWR, 0.5f, 3))
		return 1;
	electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	return 0;
}
