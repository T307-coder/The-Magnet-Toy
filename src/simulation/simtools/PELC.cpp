#include "simulation/ToolCommon.h"

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength);

void SimTool::Tool_PELC()
{
	Identifier = "DEFAULT_TOOL_PELC";
	Name = "PELC";
	Colour = 0xFFDD44_rgb;
	Description = "Creates a positive electric field source.";
	Perform = &perform;
}

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength)
{
	int cx = x / CELL;
	int cy = y / CELL;
	if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
		sim->eSrc[cy][cx] += strength * 10.0f;
	return 1;
}
