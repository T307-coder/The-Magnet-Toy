#include "simulation/ToolCommon.h"

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength);

void SimTool::Tool_NELC()
{
	Identifier = "DEFAULT_TOOL_NELC";
	Name = "NELC";
	Colour = 0x4488FF_rgb;
	Description = "Creates a negative electric field source.";
	Perform = &perform;
}

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength)
{
	int cx = x / CELL;
	int cy = y / CELL;
	if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
		sim->eSrc[cy][cx] -= strength * 10.0f;
	return 1;
}
