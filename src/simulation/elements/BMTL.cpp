#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_BMTL()
{
	Identifier = "DEFAULT_PT_BMTL";
	Name = "BMTL";
	Colour = 0x505070_rgb;
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
	Hardness = 1;

	Weight = 100;

	HeatConduct = 251;
	Description = "Breakable metal. Common conductive building material, can melt and break under pressure.";

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 1.0f;
	HighPressureTransition = ST;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1273.0f;
	HighTemperatureTransition = PT_LAVA; //@ BMTL -> LAVA(BMTL)

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp>1)
	{
		parts[i].tmp--;
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((TYP(r)==PT_METL || TYP(r)==PT_IRON) && sim->rng.chance(1, 100))
					{
						//@ BMTL + METL/IRON -> 2xBMTL
						sim->part_change_type(ID(r),x+rx,y+ry,PT_BMTL);
						parts[ID(r)].tmp = (parts[i].tmp<=7) ? parts[i].tmp=1 : parts[i].tmp - sim->rng.between(0, 4);
					}
				}
			}
		}
	}
	else if (parts[i].tmp==1 && sim->rng.chance(1, 1000))
	{
		//@ BMTL -> BRMT
		parts[i].tmp = 0;
		sim->part_change_type(i,x,y,PT_BRMT);
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
				if (rt==PT_BMTL||rt==PT_TTAN||rt==PT_BMTL||rt==PT_BRMT)
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
			if (dBdt > 1.5f && sim->rng.chance(1, 5))
			{
				sim->part_change_type(i, x, y, PT_SPRK);
				parts[i].ctype = PT_BMTL;
				parts[i].life = 4;
				return 1;
			}
		}
	}
	if (parts[i].tmp3 != 0) parts[i].life = 100;
	// Strong B-field breaks BMTL -> BRMT
	if (sim->magnetismEnabled && cx>=0 && cx<XCELLS && cy>=0 && cy<YCELLS)
	{
		if (fabsf(sim->bField[cy][cx]) > 2.0f && sim->rng.chance(1, 50))
		{
			sim->part_change_type(i, x, y, PT_BRMT);
			return 1;
		}
	}
	// Strong E-field breaks BMTL -> BRMT (dielectric breakdown)
	if (sim->electricityEnabled && cx>=0 && cx<XCELLS && cy>=0 && cy<YCELLS)
	{
		if (fabsf(sim->eField[cy][cx]) > 2.0f && sim->rng.chance(1, 50))
		{
			sim->part_change_type(i, x, y, PT_BRMT);
			return 1;
		}
	}
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
