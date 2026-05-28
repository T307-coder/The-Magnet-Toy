#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_LRBD()
{
	Identifier = "DEFAULT_PT_LRBD";
	Name = "LRBD";
	Colour = 0xAAAAAA_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.15f;
	Diffusion = 0.00f;
	HotAir = 0.000001f* CFDS;
	Falldown = 2;

	Flammable = 1000;
	Explosive = 1;
	Meltable = 0;
	Hardness = 2;

	Weight = 45;

	DefaultProperties.temp = R_TEMP + 45.0f + 273.15f;
	HeatConduct = 170;
	Description = "Liquid Rubidium.";

	Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 311.0f;
	LowTemperatureTransition = PT_RBDM; //@ LRBD -> RBDM
	HighTemperature = 961.0f;
	HighTemperatureTransition = PT_FIRE; //@ LRBD -> FIRE

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Electric charging: liquid metal, contact POSC, F = q*E
	int cx = x/CELL, cy = y/CELL;
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
