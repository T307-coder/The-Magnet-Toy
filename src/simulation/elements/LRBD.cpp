#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_LRBD()
{
	Identifier = "DEFAULT_PT_LRBD";
	Name = "LRBD";
	Colour = 0xAAAAAA_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.15f;
	Diffusion = 0.00f;
	HotAir = 0.000001f* CFDS;
	Falldown = 2;

	Flammable = 1000;
	Explosive = 1;
	Meltable = 0;
	Hardness = 2;

	Weight = 45;

	DefaultProperties.temp = R_TEMP + 45.0f + 273.15f;
	HeatConduct = 170;
	Description = "Liquid Rubidium.";

	Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 311.0f;
	LowTemperatureTransition = PT_RBDM; //@ LRBD -> RBDM
	HighTemperature = 961.0f;
	HighTemperatureTransition = PT_FIRE; //@ LRBD -> FIRE

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	// Electromagnetic Lorentz force: F = q(v x B), rotates velocity, preserves |v|
	if (sim->electricityEnabled && sim->magnetismEnabled && parts[i].tmp4 != 0)
	{
		int cx = x/CELL, cy = y/CELL;
		if (cx>=0 && cy>=0 && cx<XCELLS && cy<YCELLS)
		{
			float Bz = sim->bField[cy][cx];
			if (Bz != 0.0f)
			{
				float massFactor = 1.0f / (SimulationData::CRef().elements[parts[i].type].Gravity + 0.05f);
				float dtheta = Bz * parts[i].tmp4 * 0.05f * massFactor;
				float c = cosf(dtheta), s = sinf(dtheta);
				float vx = parts[i].vx * c - parts[i].vy * s;
				float vy = parts[i].vx * s + parts[i].vy * c;
				parts[i].vx = vx; parts[i].vy = vy;
			}
		}
	}
	return 0;
}
