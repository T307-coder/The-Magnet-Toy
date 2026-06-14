#include "simulation/ElementCommon.h"
#include "simulation/MagnetismCommon.h"
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
	DefaultProperties.ctype = 0;
	Description = Localization::Ref().Tr("sim.elem.DEFAULT_PT_TTAN");

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
	// Magnetization: shared ferromagnet update (tmp3=induced, ctype=permanent)
	int cx, cy;
	magnetism_ferromagnetUpdate(sim, parts[i], x, y, parts[i].tmp3, parts[i].ctype, cx, cy);
	// Induction: only when completely unmagnetized (both induced and permanent = 0)
	if (parts[i].ctype == 0 && parts[i].tmp3 == 0 && magnetism_tryInduction(sim, i, x, y, cx, cy, parts[i].tmp2, PT_TTAN, 1.5f, 6))
		return 1;
	electricity_chargeContact(sim, parts[i], x, y, parts[i].tmp4);
	electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
	return 0;
}
