#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"
#include "simulation/MagnetismCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_BRMT()
{
	Identifier = "DEFAULT_PT_BRMT";
	Name = "BRMT";
	Colour = 0x705060_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
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
	Description = "Broken metal. Created when iron rusts or when metals break from pressure.";

	Properties = TYPE_PART|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;
	CarriesTypeIn = 1U << FIELD_CTYPE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1273.0f;
	HighTemperatureTransition = ST; //@ BRMT -> LAVA(BMTL)

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].temp > 523.15f)//250.0f+273.15f
	{
		auto tempFactor = int(1000 - ((523.15f-parts[i].temp)*2));
		if(tempFactor < 2)
			tempFactor = 2;
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (TYP(r)==PT_BREC && sim->rng.chance(1, tempFactor))
					{
						if (sim->rng.chance(1, 2))
						{
							//@ BRMT + BREC -> BRMT + THRM
							sim->create_part(ID(r), x+rx, y+ry, PT_THRM);
						}
						else
						{
							//@ BRMT + BREC -> THRM + BREC
							sim->create_part(i, x, y, PT_THRM);
							return 1;
						}
					}
				}
			}
		}
	}
	// Magnetization: contact with magnets + DEUT-style internal diffusion
	int cx = x/CELL, cy = y/CELL;
	if (sim->magnetismEnabled && cx>=0 && cx<XCELLS && cy>=0 && cy<YCELLS)
	{
		if (parts[i].temp < 773.15f)
		{
			magnetism_contactCharge(sim, parts[i], x, y, parts[i].tmp3);
			magnetism_diffuseCharge(sim, parts[i], x, y, parts[i].tmp3);
		}
		else
		{
			if (parts[i].tmp3 > 0) parts[i].tmp3 = std::max(0, parts[i].tmp3 - 5);
			else if (parts[i].tmp3 < 0) parts[i].tmp3 = std::min(0, parts[i].tmp3 + 5);
		}
		if (parts[i].tmp3 > 100) parts[i].tmp3 = 100;
		if (parts[i].tmp3 < -100) parts[i].tmp3 = -100;
		if (parts[i].tmp3 != 0)
			sim->magSrc[cy][cx] += parts[i].tmp3 * 0.02f;
	}
	// Induction: only when completely unmagnetized
	if (sim->magnetismEnabled && cx>=0 && cx<XCELLS && cy>=0 && cy<YCELLS && parts[i].tmp3 == 0)
	{
		float Bnow = sim->bField[cy][cx];
		float Bprev = (parts[i].tmp2 == 0) ? Bnow : parts[i].tmp2 / 10000.0f;
		parts[i].tmp2 = (int)(Bnow * 10000.0f);
		if (sim->prevBFieldValid)
		{
			float dBdt = fabsf(Bnow - Bprev);
			if (dBdt > 1.5f && sim->rng.chance(1, 8))
			{
				sim->part_change_type(i, x, y, PT_SPRK);
				parts[i].ctype = PT_BRMT;
				parts[i].life = 4;
				return 1;
			}
		}
	}
	if (parts[i].tmp3 != 0) parts[i].life = 100;
	// Ferromagnetic attraction (shared function)
	magnetism_ferromagneticPull(sim, parts[i], cx, cy);
	// Electric charging, force, Lorentz, diffusion (shared functions)
	electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
	electricity_applyForce(sim, parts[i], x, y, parts[i].tmp4, 0.5f);
	electricity_applyLorentz(sim, parts[i], x, y, parts[i].tmp4);
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	return 0;
}
