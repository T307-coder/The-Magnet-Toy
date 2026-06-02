#include "simulation/ElementCommon.h"
#include "simulation/MagnetismCommon.h"
#include "simulation/ElectricityCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_IRON()
{
	Identifier = "DEFAULT_PT_IRON";
	Name = "IRON";
	Colour = 0x707070_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
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
	Hardness = 49;

	Weight = 100;

	HeatConduct = 251;
	Description = "Rusts with salt, can be used for electrolysis of WATR.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1687.0f;
	HighTemperatureTransition = PT_LAVA; //@ IRON -> LAVA(IRON)

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life && parts[i].tmp3 == 0)
		return 0;
	auto tryBreak = [&]() {
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (rx || ry)
				{
					auto r = TPT_PM(x+rx, y+ry);
					switch (TYP(r))
					{
					case PT_SALT:
						//@ IRON + SALT -> BMTL + SALT
						if (sim->rng.chance(1, 47))
							return true;
						break;
					case PT_SLTW:
						//@ IRON + SLTW -> BMTL + SLTW
						if (sim->rng.chance(1, 67))
							return true;
						break;
					case PT_WATR:
						//@ IRON + WATR -> BMTL + WATR
						if (sim->rng.chance(1, 1200))
							return true;
						break;
					case PT_O2:
						//@ IRON + O2 -> BMTL + O2
						if (sim->rng.chance(1, 250))
							return true;
						break;
					case PT_LO2:
						//@ IRON + LO2 -> BMTL + LO2
						return true;
					default:
						break;
					}
				}
			}
		}
		return false;
	};
	if (tryBreak())
	{
		sim->part_change_type(i,x,y,PT_BMTL);
		parts[i].tmp = sim->rng.between(20, 29);
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
	// Induction: only when completely unmagnetized (shared function, 30-frame cooldown)
	if (parts[i].tmp3 == 0 && magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_IRON, 1.5f, 5))
		return 1;
	if (parts[i].tmp3 != 0) parts[i].life = 100;
	// Electric charging and diffusion (shared functions)
	electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	return 0;
}
