#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_SLTW()
{
	Identifier = "DEFAULT_PT_SLTW";
	Name = "SLTW";
	Colour = 0x4050F0_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 35;

	HeatConduct = 75;
	Description = "Saltwater, conducts electricity, difficult to freeze.";

	Properties = TYPE_LIQUID | PROP_CONDUCTS | PROP_LIFE_DEC | PROP_NEUTPENETRATE | PROP_PHOTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 252.05f;
	LowTemperatureTransition = PT_ICEI; //@ SLTW -> ICE(SLTW)
	HighTemperature = 383.0f;
	HighTemperatureTransition = ST;

	ASSIGN_SIM_CALLBACK(Update, update)
}

static int update(UPDATE_FUNC_ARGS)
{
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				switch (TYP(r))
				{
				case PT_SALT:
					//@ SLTW + SALT -> 2xSLTW
					if (rng.chance(1, 2000))
						sim->part_change_type_outer(ID(r),x+rx,y+ry,PT_SLTW);
					break;
				case PT_PLNT:
					if (rng.chance(1, 40))
						sim->kill_part_outer(ID(r));
					break;
				case PT_RBDM:
				case PT_LRBD:
					//@ SLTW + RBDM/LRBD -> FIRE + RBDM/LRBD
					if ((sim->legacy_enable||parts[i].temp>(273.15f+12.0f)) && rng.chance(1, 100))
					{
						sim->part_change_type_outer(i,x,y,PT_FIRE);
						parts[i].life = 4;
						parts[i].ctype = PT_WATR;
					}
					break;
				case PT_FIRE:
					if (parts[ID(r)].ctype!=PT_WATR)
					{
						sim->kill_part_outer(ID(r));
						if (rng.chance(1, 30))
						{
							sim->kill_part_outer(i);
							return 1;
						}
					}
					break;
				case PT_NONE:
					break;
				default:
					continue;
				}
			}
		}
	}
	// Dielectrophoresis: ionic salt water strongly attracted to |E| (Gravity-weighted)
	int cx = x/CELL, cy = y/CELL;
	if (sim->electricityEnabled && cx>0 && cy>0 && cx<XCELLS-1 && cy<YCELLS-1)
	{
		float dAbsEx = fabsf(sim->eField[cy][cx+1]) - fabsf(sim->eField[cy][cx-1]);
		float dAbsEy = fabsf(sim->eField[cy+1][cx]) - fabsf(sim->eField[cy-1][cx]);
		float massFactor = 1.0f / (SimulationData::CRef().elements[parts[i].type].Gravity + 0.05f);
		parts[i].vx += dAbsEx * 10.0f * massFactor;
		parts[i].vy += dAbsEy * 10.0f * massFactor;
	}
	return 0;
}
