#include "simulation/ElementCommon.h"
#include "simulation/MagnetismCommon.h"
#include "simulation/ElectricityCommon.h"
#include "simulation/Air.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_TUNG()
{
	Identifier = "DEFAULT_PT_TUNG";
	Name = "TUNG";
	Colour = 0x505050_rgb;
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
	Description = "Tungsten. Brittle metal with a very high melting point.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 3695.0f;// TUNG melts in its update function instead of in the normal way, but store the threshold here so that it can be changed from Lua
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	bool splode = false;
	const float MELTING_POINT = elements[PT_TUNG].HighTemperature;

	if(parts[i].temp > 2400.0)
	{
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if(TYP(r) == PT_O2)
					{
						splode = true;
					}
				}
			}
		}
	}
	if((parts[i].temp > MELTING_POINT && sim->rng.chance(1, 20)) || splode)
	{
		if (sim->rng.chance(1, 50))
		{
			sim->pv[y/CELL][x/CELL] += 50.0f;
		}
		else if (sim->rng.chance(1, 100))
		{
			//@ TUNG -> FIRE
			sim->part_change_type(i, x, y, PT_FIRE);
			parts[i].life = sim->rng.between(0, 499);
			return 1;
		}
		else
		{
			//@ TUNG -> LAVA(TUNG)
			sim->part_change_type(i, x, y, PT_LAVA);
			parts[i].ctype = PT_TUNG;
			return 1;
		}
		if(splode)
		{
			parts[i].temp = restrict_flt(MELTING_POINT + sim->rng.between(200, 799), MIN_TEMP, MAX_TEMP);
		}
		parts[i].vx += sim->rng.between(-50, 50);
		parts[i].vy += sim->rng.between(-50, 50);
		return 1;
	}
	auto press = int(sim->pv[y/CELL][x/CELL] * 64);
	auto diff = press - parts[i].tmp3;
	if (diff > 32 || diff < -32)
	{
		//@ TUNG -> BRMT(TUNG)
		sim->part_change_type(i,x,y,PT_BRMT);
		parts[i].ctype = PT_TUNG;
		return 1;
	}
	parts[i].tmp3 = press;
	int cx = x/CELL, cy = y/CELL;
	if (magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_TUNG, 0.5f, 6))
		return 1;
	// Electric charging: contact POSC, store charge in tmp4
	if (sim->electricityEnabled && cx>=0 && cx<XCELLS && cy>=0 && cy<YCELLS)
	{
		for (auto rx = -1; rx <= 1; rx++)
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (!rx && !ry) continue;
				auto r = pmap[y+ry][x+rx];
				if (r)
				{
					int rt = TYP(r);
					if (rt == PT_POSC && parts[ID(r)].life==10)
					{
						int q = (int)((parts[ID(r)].temp-273.15f)/5.0f);
						if (q>100) q=100; if (q<-100) q=-100;
						if (parts[i].tmp4<q) parts[i].tmp4++; else if (parts[i].tmp4>q) parts[i].tmp4--;
					}
					else if (rt == PT_FIXC)
					{
						int q = parts[ID(r)].tmp;
						if (q>100) q=100; if (q<-100) q=-100;
						if (parts[i].tmp4<q) parts[i].tmp4++; else if (parts[i].tmp4>q) parts[i].tmp4--;
					}
				}
				auto pr = sim->photons[y+ry][x+rx];
				if (pr)
				{
					int prt = TYP(pr);
					if (prt == PT_ELEC) { if (parts[i].tmp4 > -100) parts[i].tmp4--; }
					else if (prt == PT_PROT) { if (parts[i].tmp4 < 100) parts[i].tmp4++; }
				}
			}
		if (parts[i].tmp4>100) parts[i].tmp4=100; if (parts[i].tmp4<-100) parts[i].tmp4=-100;
		if (parts[i].tmp4!=0) sim->eSrc[cy][cx] += parts[i].tmp4*0.05f;
	}
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	const float MELTING_POINT = elements[PT_TUNG].HighTemperature;
	double startTemp = (MELTING_POINT - 1500.0);
	double tempOver = (((cpart->temp - startTemp)/1500.0)*std::numbers::pi) - (std::numbers::pi/2.0);
	if (tempOver > -(std::numbers::pi / 2.0))
	{
		if (tempOver > std::numbers::pi / 2.0)
			tempOver = std::numbers::pi / 2.0;
		double gradv = sin(tempOver) + 1.0;
		*firer = (int)(gradv * 258.0);
		*fireg = (int)(gradv * 156.0);
		*fireb = (int)(gradv * 112.0);
		*firea = 30;

		*colr += *firer;
		*colg += *fireg;
		*colb += *fireb;
		*pixel_mode |= FIRE_ADD;
	}
	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp3 = int(sim->pv[y/CELL][x/CELL] * 64);
}
