#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"

int update(UPDATE_FUNC_ARGS);

void Element::Element_RSST()
{
	Identifier = "DEFAULT_PT_RSST";
	Name = "RSST";
	Colour = 0xF95B49_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.15f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 50;

	Weight = 33;

	DefaultProperties.temp = R_TEMP + 20.0f + 273.15f;
	HeatConduct = 55;
	Description = "Resist. Solidifies on contact with photons, is destroyed by electrons and spark.";

	Properties = TYPE_LIQUID | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_NEUTPASS | PROP_PHOTPASS;
	CarriesTypeIn = (1U << FIELD_CTYPE) | (1U << FIELD_TMP);

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	ASSIGN_SIM_CALLBACK(Update, update)
}

int update(UPDATE_FUNC_ARGS)
{
	for(int rx = -1; rx < 2; rx++)
	{
		for(int ry = -1; ry < 2; ry++)
		{
			auto r = pmap[y+ry][x+rx];

			if (!r)
				continue;

			//@ RSST + GUNP -> FIRW
			if(TYP(r) == PT_GUNP)
			{
				sim->create_part_outer(i, x, y, PT_FIRW);
				sim->kill_part_outer(ID(r));
				return 1;
			}

			//@ RSST + BCOL -> FSEP
			if(TYP(r) == PT_BCOL)
			{
				sim->create_part_outer(i, x, y, PT_FSEP);
				parts[i].life = 50;
				sim->kill_part_outer(ID(r));
				return 1;
			}

			// Set RSST ctype from nearby clone
			if((TYP(r) == PT_CLNE) || (TYP(r) == PT_PCLN))
			{
				if(parts[ID(r)].ctype != PT_RSST)
					parts[i].ctype = parts[ID(r)].ctype;
			}

			// Set RSST tmp from nearby breakable clone
			if((TYP(r) == PT_BCLN) || (TYP(r) == PT_PBCN))
			{
				if(parts[ID(r)].ctype != PT_RSST)
					parts[i].tmp = parts[ID(r)].ctype;
			}
		}
	}

	// Dielectrophoresis: weakly conductive resist liquid
	{
		int cx = x/CELL, cy = y/CELL;
		if (sim->electricityEnabled && cx>0 && cy>0 && cx<XCELLS-1 && cy<YCELLS-1)
		{
			float dAbsEx = fabsf(sim->eField[cy][cx+1]) - fabsf(sim->eField[cy][cx-1]);
			float dAbsEy = fabsf(sim->eField[cy+1][cx]) - fabsf(sim->eField[cy-1][cx]);
			float massFactor = 1.0f / (SimulationData::CRef().elements[parts[i].type].Gravity + 0.05f);
			parts[i].vx += dAbsEx * 2.0f * massFactor;
			parts[i].vy += dAbsEy * 2.0f * massFactor;
		}
	}

	return 0;
}
