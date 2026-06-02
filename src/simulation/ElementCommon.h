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
// Looks up the particle at (px, py) on the CURRENT particle's own Z layer.
// (sim, parts, i are all parameters of UPDATE_FUNC_ARGS, so they're in scope.)
// For full 3D cross-layer scanning, use TPT_PM3D(px, py, pz) with an rz loop.
#define TPT_PM(px, py)       sim->GetPmap3D(px, py, int(parts[i].z + 0.5f))
#define TPT_PM3D(px, py, pz) sim->GetPmap3D(px, py, pz)
#include <algorithm>
#include <cmath>
#include <numbers>
