#pragma once
// This header should be included by all files in src/elements/
#include "Misc.h"
#include "common/tpt-rand.h"
#include "ElementClasses.h"
#include "Particle.h"
#include "ElementGraphics.h"
#include "Simulation.h"
#include "SimulationData.h"
#include "graphics/Renderer.h"
#include "TransitionConstants.h"

// XYZ平等: TPT_PM is a drop-in replacement for pmap[y][x].
// Currently defaults to z=0 (2D backward compatible).
// Once an element computes int z = int(parts[i].z+0.5f), use TPT_PM3D(x,y,z).
#define TPT_PM(px, py)       sim->GetPmap3D(px, py, 0)
#define TPT_PM3D(px, py, pz) sim->GetPmap3D(px, py, pz)
#include <algorithm>
#include <cmath>
#include <numbers>
