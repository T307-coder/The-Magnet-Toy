#include "simulation/ToolCommon.h"

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength);

void SimTool::Tool_PMAG()
{
	Identifier = "DEFAULT_TOOL_PMAG";
	Name = "PMAG";
	Colour = 0xFF4444_rgb;
	Description = "Creates a positive (N) magnetic field source.";
	Perform = &perform;
}

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength)
{
	int cx = x / CELL;
	int cy = y / CELL;
	if (cx >= 0 && cy >= 0 && cx < XCELLS && cy < YCELLS)
		sim->magSrc[cy][cx] += strength * 10.0f;
	return 1;
}
