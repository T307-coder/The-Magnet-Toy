#pragma once
#include <SDL.h>
class Simulation;
bool CubeTest_Init();
void CubeTest_Render();
void CubeTest_Rotate(int dx, int dy); // mouse delta → camera orbit
void CubeTest_RotateView(int dir);     // 0=up 1=down 2=left 3=right, 90° relative to current view
void CubeTest_AdjustLayer(int delta); // Z held → change selectedLayer
void CubeTest_SetBrush(int x, int y, int rx, int ry); // update 3D brush preview
void CubeTest_SetBrushPos(int x, int y);  // mouse move → brush position (auto-clamped)
void CubeTest_SetBrushRadius(int rx, int ry); // brush changed → radius
void CubeTest_ToggleZGrid(); // G key toggles Z-axis grid lines
void CubeTest_ResetView();   // R key resets camera to front view
void CubeTest_ToggleFullscreen(); // F key toggles fullscreen
void CubeTest_Zoom(int delta);    // Ctrl+scroll: zoom 3D camera
void CubeTest_ResizeBrush(int delta); // scroll / []: resize 3D brush
void CubeTest_ToggleBrushShape(); // Tab: cube ↔ sphere
int  CubeTest_GetPlacementZ(); // returns Z for particle placement based on view+brush
void CubeTest_SetActiveTool(int toolType); // set tool type for 3D-window clicks
void CubeTest_SetSimulation(const Simulation *sim);
void CubeTest_HandleEvent(const SDL_Event &e);
void CubeTest_Shutdown();
bool CubeTest_IsOpen();
void CubeTest_ToggleCamControl(); // 'C' key toggles camera-drag mode
Uint32 CubeTest_GetWindowID();    // SDL window ID of 3D viewport, 0 if closed
