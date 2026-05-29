#include "simulation/ElementCommon.h"
#include "simulation/MagnetismCommon.h"
#include "simulation/ElectricityCommon.h"
#include "simulation/Air.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_GOLD()
{
	Identifier = "DEFAULT_PT_GOLD";
	Name = "GOLD";
	Colour = 0xDCAD2C_rgb;
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
	Hardness = 0;
	PhotonReflectWavelengths = 0x3C038100;

	Weight = 100;

	HeatConduct = 251;
	Description = "Corrosion resistant metal, will reverse corrosion of iron. Excellent conductor.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_HOT_GLOW|PROP_LIFE_DEC|PROP_NEUTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1337.0f;
	HighTemperatureTransition = PT_LAVA; //@ GOLD -> LAVA(GOLD)

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	static int checkCoordsX[] = { -4, 4, 0, 0 };
	static int checkCoordsY[] = { 0, 0, -4, 4 };
	//Find nearby rusted iron (BMTL with tmp 1+)
	for(int j = 0; j < 8; j++)
	{
		auto rndstore = sim->rng.gen();
		auto rx = (rndstore % 9)-4;
		rndstore >>= 4;
		auto ry = (rndstore % 9)-4;
		if ((!rx != !ry)) {
			auto r = pmap[y+ry][x+rx];
			if(!r) continue;
			if(TYP(r)==PT_BMTL && parts[ID(r)].tmp)
			{
				//@ GOLD + BMTL -> GOLD + IRON
				parts[ID(r)].tmp = 0;
				sim->part_change_type(ID(r), x+rx, y+ry, PT_IRON);
			}
		}
	}
	//Find sparks
	if(!parts[i].life)
	{
		for(int j = 0; j < 4; j++)
		{
			auto rx = checkCoordsX[j];
			auto ry = checkCoordsY[j];
			auto r = pmap[y+ry][x+rx];
			if(!r) continue;
			if(TYP(r)==PT_SPRK && parts[ID(r)].life && parts[ID(r)].life<4)
			{
				sim->part_change_type(i, x, y, PT_SPRK);
				parts[i].life = 4;
				parts[i].ctype = PT_GOLD;
			}
		}
	}
	if (TYP(sim->photons[y][x]) == PT_NEUT)
	{
		if (sim->rng.chance(1, 7))
		{
			sim->kill_part(ID(sim->photons[y][x]));
		}
	}
	int cx = x/CELL, cy = y/CELL;
	if (magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_GOLD, 0.5f, 4))
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
	// Charge diffusion: equalize between conductors (DEUT-style)
	for (auto trade = 0; trade < 4; trade++)
	{
		auto rx = sim->rng.between(-2,2), ry = sim->rng.between(-2,2);
		if (!rx && !ry) continue;
		auto r = pmap[y+ry][x+rx];
		if (r && (SimulationData::CRef().elements[TYP(r)].Properties & PROP_CONDUCTS))
		{
			int diff = parts[i].tmp4 - parts[ID(r)].tmp4;
			if (diff > 1) { int t = diff/2; parts[ID(r)].tmp4 += t; parts[i].tmp4 -= t; }
			else if (diff == 1) { parts[ID(r)].tmp4++; parts[i].tmp4--; }
		}
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int rndstore = gfctx.rng.gen();
	*colr += (rndstore % 10) - 5;
	rndstore >>= 4;
	*colg += (rndstore % 10)- 5;
	rndstore >>= 4;
	*colb += (rndstore % 10) - 5;
	return 0;
}
