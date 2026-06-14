#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_THRM()
{
	Identifier = "DEFAULT_PT_THRM";
	Name = "THRM";
	Colour = 0xA08090_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.3f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 2;
	Hardness = 2;

	Weight = 90;

	HeatConduct = 211;
	Description = Localization::Ref().Tr("sim.elem.DEFAULT_PT_THRM");

	Properties = TYPE_PART|PROP_CONDUCTS;

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
	// Thermite EM response: hot enough to ionise (>2000°C), conductive molten metal
	if (parts[i].temp > 2273.15f)
	{
		electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
		electricity_polarizeCharge(sim, parts[i], x, y, parts[i].tmp4);
		electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
		if (parts[i].tmp4 != 0 && sim->freeChargeFieldsEnabled)
		{
			int cx = x / CELL, cy = y / CELL;
			if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
				sim->eSrc[cy][cx] += parts[i].tmp4 * 0.05f;
		}
		electricity_applyForce(sim, parts[i], x, y, parts[i].tmp4, 0.5f);
		electricity_applyLorentz(sim, parts[i], x, y, parts[i].tmp4);
	}
	return 0;
}
