#include "simulation/ElementCommon.h"
#include "simulation/ElectricityCommon.h"
#include "FIRE.h"

static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);
static int update(UPDATE_FUNC_ARGS);

void Element::Element_PLSM()
{
	Identifier = "DEFAULT_PT_PLSM";
	Name = "PLSM";
	Colour = 0xBB99FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.9f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.20f;
	Collision = 0.0f;
	Gravity = -0.1f;
	Diffusion = 0.30f;
	HotAir = 0.001f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = MAX_TEMP;
	HeatConduct = 5;
	Description = Localization::Ref().Tr("sim.elem.DEFAULT_PT_PLSM");

	Properties = TYPE_GAS|PROP_LIFE_DEC|PROP_CONDUCTS;
	CarriesTypeIn = 1U << FIELD_CTYPE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	RGB color = Renderer::plasmaTableAt(cpart->life);
	*colr = color.Red;
	*colg = color.Green;
	*colb = color.Blue;

	*firea = 255;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	*pixel_mode = PMODE_GLOW | PMODE_ADD; //Clear default, don't draw pixel
	*pixel_mode |= FIRE_ADD;
	//Returning 0 means dynamic, do not cache
	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = sim->rng.between(50, 199);
}

static int update(UPDATE_FUNC_ARGS)
{
	// Run standard FIRE update first (heat, ignition, lava interactions)
	Element_FIRE_update(UPDATE_FUNC_SUBCALL_ARGS);

	// Plasma electromagnetic response: polarization + diffusion + DEP + Lorentz
	if (parts[i].type == PT_PLSM)
	{
		// Use tmp4 as charge storage (FIRE update doesn't use it)
		electricity_polarizeCharge(sim, parts[i], x, y, parts[i].tmp4);
		electricity_diffuseCharge(sim, parts[i], x, y, parts[i].tmp4);
		// Plasma charge feeds back into E-field via eSrc
		if (parts[i].tmp4 != 0 && sim->freeChargeFieldsEnabled)
		{
			int cx = x / CELL, cy = y / CELL;
			if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
				sim->eSrc[cy][cx] += parts[i].tmp4 * 0.05f;
		}
		electricity_applyForce(sim, parts[i], x, y, parts[i].tmp4, 1.0f);
		electricity_applyLorentz(sim, parts[i], x, y, parts[i].tmp4);
	}
	return 0;
}
