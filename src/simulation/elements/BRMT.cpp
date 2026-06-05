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
							//@ BRMT + BREC -> BRMT + THRM (inherit charge from BREC)
							int charge = sim->parts[ID(r)].tmp4;
							int np = sim->create_part(ID(r), x+rx, y+ry, PT_THRM);
							if (np >= 0) sim->parts[np].tmp4 = charge;
						}
						else
						{
							//@ BRMT + BREC -> THRM + BREC (inherit charge from BRMT)
							int charge = parts[i].tmp4;
							int np = sim->create_part(i, x, y, PT_THRM);
							if (np >= 0) sim->parts[np].tmp4 = charge;
							return 1;
						}
					}
				}
			}
		}
	}
	// Magnetization: shared ferromagnet update (contact, diffusion, decay, magSrc)
	int cx, cy;
	magnetism_ferromagnetUpdate(sim, parts[i], x, y, parts[i].tmp3, cx, cy);
	// Induction: shared function handles cooldown, tmp3 tag, Biot-Savart skip (old, not recommended)
	if (parts[i].tmp3 == 0)
		magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_BRMT, 1.5f, 8);
	// Ferromagnetic attraction (shared function)
	magnetism_ferromagneticPull(sim, parts[i], cx, cy);
	// Electric charging, force, Lorentz, diffusion (shared functions)
	electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
	electricity_applyForce(sim, parts[i], x, y, parts[i].tmp4, 0.5f);
	electricity_applyLorentz(sim, parts[i], x, y, parts[i].tmp4);
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	return 0;
}
