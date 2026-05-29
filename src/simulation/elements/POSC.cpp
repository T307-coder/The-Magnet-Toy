#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_POSC()
{
	Identifier = "DEFAULT_PT_POSC";
	Name = "POSC";
	Colour = 0xFFAA20_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWERED;
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
	Meltable = 0;
	Hardness = 1;

	Weight = 100;

	HeatConduct = 0;
	Description = "Electrode plate. Temp>0C=positive(yellow), Temp<0C=negative(blue). Use HEAT/COOL.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	DefaultProperties.life = 10;

	ASSIGN_SIM_CALLBACK(Update, update)
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life != 10)
	{
		if (parts[i].life > 0)
			parts[i].life--;
	}
	else
	{
		if (parts[i].temp >= 256.0f + 273.15f)
			parts[i].temp = 256.0f + 273.15f;
		if (parts[i].temp <= -256.0f + 273.15f)
			parts[i].temp = -256.0f + 273.15f;

		sim->eSrc[y / CELL][x / CELL] = 0.2f * (parts[i].temp - 273.15);
		for (auto rx = -2; rx <= 2; rx++)
		{
			for (auto ry = -2; ry <= 2; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y + ry][x + rx];
					if (!r)
						continue;
					if (TYP(r) == PT_POSC)
					{
						if (parts[ID(r)].life < 10 && parts[ID(r)].life > 0)
							parts[i].life = 9;
						else if (parts[ID(r)].life == 0)
							parts[ID(r)].life = 10;
					}
				}
			}
		}
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life == 10)
	{
		float dt = cpart->temp - 273.15f;
		if (dt > 0) { *colr = 255; *colg = 200 - (int)(dt*0.6f); *colb = 50; }
		else        { *colr = 50; *colg = 200 + (int)(dt*0.6f); *colb = 255; }
	}
	else
	{
		*colr = 60; *colg = 40; *colb = 60;
	}
	return 0;
}
