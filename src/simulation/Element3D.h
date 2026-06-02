#pragma once
#include "simulation/Simulation.h"

// XYZ平等 helper: provides 3D-aware particle lookup for element Update callbacks.
// Usage in element files:
//   1. #include "simulation/Element3D.h"
//   2. Replace: pmap[y+ry][x+rx]   →   TPT_PARTICLE(x+rx, y+ry)
//
// TPT_PARTICLE uses GetPmap3D at the particle's own Z layer.
// For full 3D neighbour scan, use TPT_PARTICLE_AT(x, y, z).

// Lookup at current particle's Z layer
#define TPT_PARTICLE(px, py) \
	sim->GetPmap3D(px, py, _tptZ)

// Full 3D lookup at specific Z
#define TPT_PARTICLE_AT(px, py, pz) \
	sim->GetPmap3D(px, py, pz)

// Declare _tptZ at the start of the update function.
// Place this right after computing x, y:
//   int x = (int)(parts[i].x+0.5f), y = (int)(parts[i].y+0.5f);
//   TPT_INIT_Z;
#define TPT_INIT_Z \
	int _tptZ = int(parts[i].z + 0.5f)
