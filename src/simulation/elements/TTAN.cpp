#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"
#include "simulation/Air.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_TTAN()
{
	Identifier = "DEFAULT_PT_TTAN";
	Name = "TTAN";
	Colour = 0x909090_rgb;
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
	Hardness = 48;

	Weight = 100;

	HeatConduct = 251;
	Description = "Titanium. Higher melting temperature than most other metals, blocks all air pressure.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_HOT_GLOW|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1941.0f;
	HighTemperatureTransition = PT_LAVA; //@ TTAN -> LAVA(TTAN)

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	int ttan = 0;
	if (nt <= 2)
		ttan = 2;
	else if (parts[i].tmp)
		ttan = 2;
	else if (nt <= 6)
	{
		for (int rx = -1; rx <= 1; rx++)
		{
			for (int ry = -1; ry <= 1; ry++)
			{
				if (!rx != !ry)
				{
					if (TYP(pmap[y+ry][x+rx]) == PT_TTAN)
						ttan++;
				}
			}
		}
	}

	if (ttan >= 2)
	{
		sim->air->bmap_blockair[y/CELL][x/CELL] = 1;
		sim->air->bmap_blockairh[y/CELL][x/CELL] = 0x8;
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
				if (rt==PT_TTAN||rt==PT_TTAN||rt==PT_BMTL||rt==PT_BRMT)
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
	// Induction: only when completely unmagnetized
	if (sim->magnetismEnabled && cx>=0 && cx<XCELLS && cy>=0 && cy<YCELLS && parts[i].tmp3 == 0)
	{
		float Bnow = sim->bField[cy][cx];
		float Bprev = (parts[i].tmp2 == 0) ? Bnow : parts[i].tmp2 / 10000.0f;
		parts[i].tmp2 = (int)(Bnow * 10000.0f);
		if (sim->prevBFieldValid)
		{
			float dBdt = fabsf(Bnow - Bprev);
			if (dBdt > 1.5f && sim->rng.chance(1, 6))
			{
				sim->part_change_type(i, x, y, PT_SPRK);
				parts[i].ctype = PT_TTAN;
				parts[i].life = 4;
				return 1;
			}
		}
	}
	if (parts[i].tmp3 != 0) parts[i].life = 100;
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
