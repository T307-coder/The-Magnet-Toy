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
					auto r = pmap[y+ry][x+rx];
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
			// Contact charging: MAGN, ELMG, MGPN
			for (auto rx = -1; rx <= 1; rx++)
				for (auto ry = -1; ry <= 1; ry++)
				{
					if (!rx && !ry) continue;
					auto r = pmap[y+ry][x+rx];
					if (r)
					{
						int rt = TYP(r);
						if (rt == PT_MAGN)
						{
							int mag = parts[ID(r)].tmp;
							if (parts[i].tmp3 < mag) parts[i].tmp3++;
							else if (parts[i].tmp3 > mag) parts[i].tmp3--;
						}
						else if (rt == PT_ELMG && parts[ID(r)].life == 10)
						{
							int mag = (int)((parts[ID(r)].temp - 273.15f) / 5.0f);
							if (mag > 100) mag = 100;
							if (mag < -100) mag = -100;
							if (parts[i].tmp3 < mag) parts[i].tmp3++;
							else if (parts[i].tmp3 > mag) parts[i].tmp3--;
						}
					}
					auto pr = sim->photons[y+ry][x+rx];
					if (pr && TYP(pr) == PT_MGPN)
					{
						int mag = parts[ID(pr)].tmp;
						if (parts[i].tmp3 < mag) parts[i].tmp3++;
						else if (parts[i].tmp3 > mag) parts[i].tmp3--;
					}
				}
			// Internal diffusion: DEUT-style random trade (conserves total)
			for (auto trade = 0; trade < 4; trade++)
			{
				auto rx = sim->rng.between(-2, 2);
				auto ry = sim->rng.between(-2, 2);
				if (!rx && !ry) continue;
				auto r = pmap[y+ry][x+rx];
				if (!r) continue;
				int rt = TYP(r);
				if (rt==PT_IRON||rt==PT_TTAN||rt==PT_BMTL||rt==PT_BRMT)
				{
					int diff = parts[i].tmp3 - parts[ID(r)].tmp3;
					if (diff > 1)
					{
						int transfer = diff / 2;
						parts[ID(r)].tmp3 += transfer;
						parts[i].tmp3 -= transfer;
					}
					else if (diff == 1)
					{
						parts[ID(r)].tmp3++;
						parts[i].tmp3--;
					}
				}
			}
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
	if (parts[i].tmp3 == 0 && magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_IRON, 1.5f, 5, 30))
		return 1;
	if (parts[i].tmp3 != 0) parts[i].life = 100;
	// Electric charging and diffusion (shared functions)
	electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	return 0;
}
