#include "simulation/ElementCommon.h"

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
						else //@ BRMT + BREC -> THRM + BREC
							sim->create_part(i, x, y, PT_THRM);
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
				if (rt==PT_BRMT||rt==PT_TTAN||rt==PT_BMTL||rt==PT_BRMT)
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
	// Ferromagnetic attraction: pulled toward stronger |B|
	if (sim->magnetismEnabled && cx > 0 && cy > 0 && cx < XCELLS - 1 && cy < YCELLS - 1)
	{
		float dAbsBx = fabsf(sim->bField[cy][cx + 1]) - fabsf(sim->bField[cy][cx - 1]);
		float dAbsBy = fabsf(sim->bField[cy + 1][cx]) - fabsf(sim->bField[cy - 1][cx]);
		float massFactor = 1.0f / (SimulationData::CRef().elements[parts[i].type].Gravity + 0.05f);
		parts[i].vx += dAbsBx * 0.5f * massFactor;
		parts[i].vy += dAbsBy * 0.5f * massFactor;
	}
	// Electric charging: contact POSC, F = q*E
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
				parts[i].vx += dAbsEx*0.5f * massFactor;
				parts[i].vy += dAbsEy*0.5f * massFactor;
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
