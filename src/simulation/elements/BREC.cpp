#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_BREC()
{
	Identifier = "DEFAULT_PT_BREC";
	Name = "BREL";
	Colour = 0x707060_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.18f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 2;
	Hardness = 2;

	Weight = 90;

	HeatConduct = 211;
	Description = "Broken electronics. Formed from EMP blasts, and when constantly sparked while under pressure, turns to EXOT.";

	Properties = TYPE_PART|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life)
	{
		if (sim->pv[y/CELL][x/CELL]>10.0f)
		{
			if (parts[i].temp>9000 && sim->pv[y/CELL][x/CELL]>30.0f && sim->rng.chance(1, 200))
			{
				//@ BREC -> EXOT
				sim->part_change_type(i, x, y, PT_EXOT);
				parts[i].life = 1000;
			}
			parts[i].temp += (sim->pv[y/CELL][x/CELL])/8;
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
				parts[i].ctype = PT_BREC;
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
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
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
