#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_MERC()
{
	Identifier = "DEFAULT_PT_MERC";
	Name = "MERC";
	Colour = 0x736B6D_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.3f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 18;

	Weight = 91;

	HeatConduct = 251;
	Description = "Mercury. Volume changes with temperature, Conductive.";

	Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_NEUTABSORB|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	DefaultProperties.tmp = 10;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Max number of particles that can be condensed into one
	const int absorbScale = 10000;
	// Obscure division by 0 fix
	if (parts[i].temp + 1 == 0)
		parts[i].temp = 0;
	int maxtmp = int(absorbScale/(parts[i].temp + 1))-1;
	if (sim->rng.chance(absorbScale%(int(parts[i].temp)+1), int(parts[i].temp)+1))
		maxtmp ++;

	if (parts[i].tmp < 0)
	{
		parts[i].tmp = 0;
	}
	if (parts[i].tmp > absorbScale)
	{
		parts[i].tmp = absorbScale;
	}

	if (parts[i].tmp < maxtmp)
	{
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (!r || (parts[i].tmp >=maxtmp))
						continue;
					if (TYP(r)==PT_MERC&& sim->rng.chance(1, 3))
					{
						if ((parts[i].tmp + parts[ID(r)].tmp + 1) <= maxtmp)
						{
							parts[i].tmp += parts[ID(r)].tmp + 1;
							sim->kill_part(ID(r));
						}
					}
				}
			}
		}
	}
	else
	{
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (parts[i].tmp<=maxtmp)
						continue;
					if ((!r)&&parts[i].tmp>=1)//if nothing then create MERC
					{
						auto np = sim->create_part(-1,x+rx,y+ry,PT_MERC);
						if (np<0) continue;
						parts[i].tmp--;
						parts[np].temp = parts[i].temp;
						parts[np].tmp = 0;
						parts[np].dcolour = parts[i].dcolour;
					}
				}
			}
		}
	}
	for (auto trade = 0; trade<4; trade ++)
	{
		auto rx = sim->rng.between(-2, 2);
		auto ry = sim->rng.between(-2, 2);
		if (rx || ry)
		{
			auto r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if (TYP(r)==PT_MERC&&(parts[i].tmp>parts[ID(r)].tmp)&&parts[i].tmp>0)//diffusion
			{
				int temp = parts[i].tmp - parts[ID(r)].tmp;
				if (temp ==1)
				{
					parts[ID(r)].tmp ++;
					parts[i].tmp --;
				}
				else if (temp>0)
				{
					parts[ID(r)].tmp += temp/2;
					parts[i].tmp -= temp/2;
				}
			}
		}
	}
	int cx = x/CELL, cy = y/CELL;
	if (sim->magnetismEnabled && cx>=0 && cx<XCELLS && cy>=0 && cy<YCELLS)
	{
		float Bnow = sim->bField[cy][cx];
		float Bprev = parts[i].tmp2 / 10000.0f;
		parts[i].tmp2 = (int)(Bnow * 10000.0f);
		if (sim->prevBFieldValid)
		{
			float dBdt = fabsf(Bnow - Bprev);
			if (dBdt > 0.5f && sim->rng.chance(1, 8))
			{
				sim->part_change_type(i, x, y, PT_SPRK);
				parts[i].ctype = PT_MERC;
				parts[i].life = 4;
				return 1;
			}
		}
	}
	// Ferromagnetic attraction: pulled toward stronger |B|
	if (sim->magnetismEnabled && cx > 0 && cy > 0 && cx < XCELLS - 1 && cy < YCELLS - 1)
	{
		float dAbsBx = fabsf(sim->bField[cy][cx + 1]) - fabsf(sim->bField[cy][cx - 1]);
		float dAbsBy = fabsf(sim->bField[cy + 1][cx]) - fabsf(sim->bField[cy - 1][cx]);
		float massFactor = 1.0f / (SimulationData::CRef().elements[parts[i].type].Gravity + 0.05f);
		parts[i].vx += dAbsBx * 0.5f * massFactor;
		parts[i].vy += dAbsBy * 0.5f * massFactor;
	}
	// Electric charging from POSC contact + E-field force
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
		if (cx>0 && cy>0 && cx<XCELLS-1 && cy<YCELLS-1)
		{
			float dEx=sim->eField[cy][cx+1]-sim->eField[cy][cx-1];
			float dEy=sim->eField[cy+1][cx]-sim->eField[cy-1][cx];
			float massFactor = 1.0f / (SimulationData::CRef().elements[parts[i].type].Gravity + 0.05f);
			if (parts[i].tmp4 != 0)
			{
				parts[i].vx -= dEx*parts[i].tmp4*0.5f * massFactor;
				parts[i].vy -= dEy*parts[i].tmp4*0.5f * massFactor;
			}
			else
			{
				float dAbsEx=fabsf(sim->eField[cy][cx+1])-fabsf(sim->eField[cy][cx-1]);
				float dAbsEy=fabsf(sim->eField[cy+1][cx])-fabsf(sim->eField[cy-1][cx]);
				parts[i].vx += dAbsEx*1.0f * massFactor;
				parts[i].vy += dAbsEy*1.0f * massFactor;
			}
		}
		if (parts[i].tmp4>100) parts[i].tmp4=100; if (parts[i].tmp4<-100) parts[i].tmp4=-100;
		if (parts[i].tmp4!=0) sim->eSrc[cy][cx] += parts[i].tmp4*0.02f;
	}	// Charge diffusion: equalize between conductors (DEUT-style)
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
	// Electromagnetic Lorentz force: F = q(v x B), rotates velocity, preserves |v|
	if (sim->electricityEnabled && sim->magnetismEnabled && parts[i].tmp4 != 0)
	{
		int cx = x/CELL, cy = y/CELL;
		if (cx>=0 && cy>=0 && cx<XCELLS && cy<YCELLS)
		{
			float Bz = sim->bField[cy][cx];
			if (Bz != 0.0f)
			{
				float massFactor = 1.0f / (SimulationData::CRef().elements[parts[i].type].Gravity + 0.05f);
				float dtheta = Bz * parts[i].tmp4 * 0.05f * massFactor;
				float c = cosf(dtheta), s = sinf(dtheta);
				float vx = parts[i].vx * c - parts[i].vy * s;
				float vy = parts[i].vx * s + parts[i].vy * c;
				parts[i].vx = vx; parts[i].vy = vy;
			}
		}
	}
	return 0;
}
