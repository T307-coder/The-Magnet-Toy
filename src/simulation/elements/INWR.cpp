#include "simulation/ElementCommon.h"
#include "simulation/MagnetismCommon.h"
#include "simulation/ElectricityCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_INWR()
{
	Identifier = "DEFAULT_PT_INWR";
	Name = "INWR";
	Colour = 0x544141_rgb;
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
	Description = "Insulated wire. Only conducts to PSCN, NSCN, WIFI, and SWCH.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1687.0f;
	HighTemperatureTransition = PT_LAVA; //@ INWR -> LAVA(INWR)

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	int cx = x/CELL, cy = y/CELL;
	if (magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_INWR, 0.5f, 3))
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
