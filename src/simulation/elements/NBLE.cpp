#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_NBLE()
{
	Identifier = "DEFAULT_PT_NBLE";
	Name = "NBLE";
	Colour = 0xEB4917_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 1.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.0f;
	Diffusion = 0.75f;
	HotAir = 0.001f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;
	PhotonReflectWavelengths = 0x3FFF8000;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 2.0f + 273.15f;
	HeatConduct = 106;
	Description = "Noble Gas. Ionizes into plasma when sparked. Diffuses.";

	Properties = TYPE_GAS|PROP_CONDUCTS|PROP_LIFE_DEC;

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
	auto &sd = SimulationData::CRef();
	auto &can_move = sd.can_move;
	if (parts[i].temp > 5273.15 && sim->pv[y/CELL][x/CELL] > 100.0f)
	{
		parts[i].tmp |= 0x1;
		//@ NBLE -> CO2 + NEUT + PHOT + sometimes ELEC + PLSM
		if (sim->rng.chance(1, 5))
		{
			int j;
			float temp = parts[i].temp;
			sim->create_part(i,x,y,PT_CO2);

			j = sim->create_part(-3,x,y,PT_NEUT);
			if (j != -1)
				parts[j].temp = temp;
			if (sim->rng.chance(1, 25))
			{
				j = sim->create_part(-3,x,y,PT_ELEC);
				if (j != -1)
					parts[j].temp = temp;
			}
			j = sim->create_part(-3,x,y,PT_PHOT);
			if (j != -1)
			{
				parts[j].ctype = 0xF800000;
				parts[j].temp = temp;
				parts[j].tmp = 0x1;
			}
			int rx = x + sim->rng.between(-1, 1), ry = y + sim->rng.between(-1, 1), rt = TYP(pmap[ry][rx]);
			if (can_move[PT_PLSM][rt] || rt == PT_NBLE)
			{
				j = sim->create_part(-3,rx,ry,PT_PLSM);
				if (j != -1)
				{
					parts[j].temp = temp;
					parts[j].tmp |= 4;
				}
			}
			parts[i].temp = temp + 1750 + sim->rng.between(0, 499);
			sim->pv[y/CELL][x/CELL] += 50;
		}
	}
	// Plasma EM response: ionization above 2000°C
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
