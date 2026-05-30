// SDL2+OpenGL 3D particle viewport
#include "CubeTest.h"
#include "SimulationConfig.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <cmath>
#include <cstdio>

static SDL_Window *g_win = nullptr;
static SDL_GLContext g_gl = nullptr;
static const Simulation *g_sim = nullptr;
bool g_camControl = false;
static int g_w = 640, g_h = 480;
static float g_rotX = 0.0f, g_rotY = 0.0f, g_dist = 800.0f;
static bool g_mouseDown = false;
static int g_mx = 0, g_my = 0;
#ifdef _WIN32
static GLuint g_fontBase = 0;
#endif
static int g_brushX = -1, g_brushY = -1;
static int g_brushRX = 4, g_brushRY = 4;
static bool g_showZGrid = false;
// View mode and full 3D brush position
static int g_viewMode = 0;
static float g_brushPX = 0, g_brushPY = 0, g_brushPZ = 0;
static int g_activeToolType = 0; // current tool element type for 3D clicks
static bool g_placing = false;   // left button held → continuous placement
static const int ZMAX = 384;     // Z extent (matches YRES for cubic volume)
// Cached matrices for gluUnProject
static double g_proj[16], g_modelview[16];
static int    g_viewport[4];

#ifdef _WIN32
bool CubeTest_Init()
{
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	g_win = SDL_CreateWindow("3D TPT View",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		g_w, g_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!g_win) return false;
	g_gl = SDL_GL_CreateContext(g_win);
	if (!g_gl) return false;
	glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glPointSize(3.0f);
	HDC hdc = wglGetCurrentDC();
	HFONT hFont = CreateFontA(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		FF_DONTCARE | DEFAULT_PITCH, "Arial");
	SelectObject(hdc, hFont);
	g_fontBase = glGenLists(128);
	wglUseFontBitmaps(hdc, 0, 128, g_fontBase);
	DeleteObject(hFont);
	printf("3D Viewport: OpenGL %s ready\n", glGetString(GL_VERSION));
	return true;
}
#else
bool CubeTest_Init() { return false; }
#endif

void CubeTest_SetSimulation(const Simulation *sim) { g_sim = sim; }

void CubeTest_Render()
{
	if (!g_win || !g_gl) return;
	const Simulation *sim = g_sim;

	SDL_GL_MakeCurrent(g_win, g_gl);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, g_w, g_h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float aspect = (float)g_w / (float)g_h;
	gluPerspective(45.0, aspect, 10.0, 10000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -g_dist);
	glRotatef(g_rotX, 1, 0, 0);
	glRotatef(g_rotY, 0, 1, 0);
	// Pivot to center the 3D volume (X:0..XRES, Y:0..YRES, Z:0..ZMAX)
	// Y flipped so TPT Y=0 is at screen top
	glTranslatef(-XRES / 2.0f, YRES / 2.0f, -ZMAX / 2.0f);
	glScalef(1, -1, 1);

	// Cache matrices for gluUnProject (3D window mouse → world coords)
	glGetDoublev(GL_PROJECTION_MATRIX, g_proj);
	glGetDoublev(GL_MODELVIEW_MATRIX, g_modelview);
	glGetIntegerv(GL_VIEWPORT, g_viewport);

	// ---- Three orthogonal grid planes (XY floor, XZ back, YZ left) ----
	glBegin(GL_LINES);
	// XY plane (floor): Z=0
	glColor3f(0.25f, 0.25f, 0.35f);
	for (int x = 0; x <= XRES; x += 50)
	{ glVertex3f((float)x, 0, 0); glVertex3f((float)x, (float)YRES, 0); }
	for (int y = 0; y <= YRES; y += 50)
	{ glVertex3f(0, (float)y, 0); glVertex3f((float)XRES, (float)y, 0); }
	// XZ plane (back wall): Y=0
	glColor3f(0.20f, 0.28f, 0.28f);
	for (int x = 0; x <= XRES; x += 50)
	{ glVertex3f((float)x, 0, 0); glVertex3f((float)x, 0, (float)ZMAX); }
	for (int z = 0; z <= ZMAX; z += 50)
	{ glVertex3f(0, 0, (float)z); glVertex3f((float)XRES, 0, (float)z); }
	// YZ plane (left wall): X=0
	glColor3f(0.28f, 0.20f, 0.28f);
	for (int y = 0; y <= YRES; y += 50)
	{ glVertex3f(0, (float)y, 0); glVertex3f(0, (float)y, (float)ZMAX); }
	for (int z = 0; z <= ZMAX; z += 50)
	{ glVertex3f(0, 0, (float)z); glVertex3f(0, (float)YRES, (float)z); }
	glEnd();

	// XYZ axes from origin. X→right, Y→up (3D up = -TPT Y), Z→out of screen
	const float axLen = 60.0f;
	const float headLen = 10.0f;
	const float headW = 4.0f;
	glBegin(GL_LINES);
	// X axis �?red �?+X (right)
	glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f( axLen, 0, 0);
	glVertex3f(axLen, 0, 0); glVertex3f(axLen-headLen,  headW, 0);
	glVertex3f(axLen, 0, 0); glVertex3f(axLen-headLen, -headW, 0);
	glVertex3f(axLen, 0, 0); glVertex3f(axLen-headLen, 0,  headW);
	glVertex3f(axLen, 0, 0); glVertex3f(axLen-headLen, 0, -headW);
	// Y axis �?green �?+Y (down, same direction as TPT Y)
	glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, axLen, 0);
	glVertex3f(0, axLen, 0); glVertex3f( headW, axLen-headLen, 0);
	glVertex3f(0, axLen, 0); glVertex3f(-headW, axLen-headLen, 0);
	glVertex3f(0, axLen, 0); glVertex3f(0, axLen-headLen,  headW);
	glVertex3f(0, axLen, 0); glVertex3f(0, axLen-headLen, -headW);
	// Z axis �?blue �?+Z (out of screen)
	glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, axLen);
	glVertex3f(0, 0, axLen); glVertex3f( headW, 0, axLen-headLen);
	glVertex3f(0, 0, axLen); glVertex3f(-headW, 0, axLen-headLen);
	glVertex3f(0, 0, axLen); glVertex3f(0,  headW, axLen-headLen);
	glVertex3f(0, 0, axLen); glVertex3f(0, -headW, axLen-headLen);
	glEnd();

	// Axis labels at arrow tips
	if (g_fontBase)
	{
		float lblOff = axLen + 8.0f;
		glListBase(g_fontBase);
		glColor3f(1, 0, 0); glRasterPos3f( lblOff, 2, 0);
		glCallLists(1, GL_UNSIGNED_BYTE, "X");
		glColor3f(0, 1, 0); glRasterPos3f(2, lblOff, 0);
		glCallLists(1, GL_UNSIGNED_BYTE, "Y");
		glColor3f(0, 0, 1); glRasterPos3f(2, 0, lblOff);
		glCallLists(1, GL_UNSIGNED_BYTE, "Z");
	}

	if (!sim) { SDL_GL_SwapWindow(g_win); return; }

	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	// ---- Z-axis grid (optional, toggled with G) ----
	if (g_showZGrid)
	{
		glColor3f(0.15f, 0.25f, 0.15f);
		glBegin(GL_LINES);
		for (int z = 0; z <= ZMAX; z += 50)
		{
			float fz = (float)z;
			for (int x = 0; x <= XRES; x += 50)
			{ glVertex3f((float)x, 0, fz); glVertex3f((float)x, (float)YRES, fz); }
			for (int y = 0; y <= YRES; y += 50)
			{ glVertex3f(0, (float)y, fz); glVertex3f((float)XRES, (float)y, fz); }
		}
		glEnd();
	}

	// ---- Brush preview: rectangle + crosshair in the active plane ----
	if (g_brushX >= 0 && g_brushY >= 0)
	{
		float bx = g_brushPX, by = g_brushPY, bz = g_brushPZ;
		float rx = (float)g_brushRX, ry = (float)g_brushRY;
		int vm = g_viewMode;

		// Yellow wireframe rectangle on the active plane
		glColor3f(1.0f, 1.0f, 0.3f);
		glBegin(GL_LINE_LOOP);
		if (vm == 0 || vm == 3) { // XY plane (Front 0, Back 3)
			glVertex3f(bx-rx, by-ry, bz); glVertex3f(bx+rx, by-ry, bz);
			glVertex3f(bx+rx, by+ry, bz); glVertex3f(bx-rx, by+ry, bz);
		} else if (vm == 1 || vm == 4) { // XZ plane (Top 1, Bottom 4)
			glVertex3f(bx-rx, by, bz-ry); glVertex3f(bx+rx, by, bz-ry);
			glVertex3f(bx+rx, by, bz+ry); glVertex3f(bx-rx, by, bz+ry);
		} else { // YZ plane (Right 2, Left 5)
			glVertex3f(bx, by-ry, bz-rx); glVertex3f(bx, by+ry, bz-rx);
			glVertex3f(bx, by+ry, bz+rx); glVertex3f(bx, by-ry, bz+rx);
		}
		glEnd();

		// Crosshair: two lines in the active plane, perpendicular to each other
		glBegin(GL_LINES);
		glColor3f(1, 1, 0.3f);
		float c = 6.0f; // crosshair half-length
		switch (vm) {
		case 0: case 3: // XY plane: cross in X and Y, lock Z
			glVertex3f(bx-c, by, bz); glVertex3f(bx+c, by, bz);
			glVertex3f(bx, by-c, bz); glVertex3f(bx, by+c, bz);
			// Locked axis: blue stub pointing in Z
			glColor3f(0.4f, 0.4f, 1.0f);
			glVertex3f(bx, by, bz-c*2); glVertex3f(bx, by, bz+c*2);
			break;
		case 1: case 4: // XZ plane: cross in X and Z, lock Y
			glVertex3f(bx-c, by, bz); glVertex3f(bx+c, by, bz);
			glVertex3f(bx, by, bz-c); glVertex3f(bx, by, bz+c);
			glColor3f(0.4f, 0.4f, 1.0f);
			glVertex3f(bx, by-c*2, bz); glVertex3f(bx, by+c*2, bz);
			break;
		default: // YZ plane: cross in Y and Z, lock X
			glVertex3f(bx, by-c, bz); glVertex3f(bx, by+c, bz);
			glVertex3f(bx, by, bz-c); glVertex3f(bx, by, bz+c);
			glColor3f(0.4f, 0.4f, 1.0f);
			glVertex3f(bx-c*2, by, bz); glVertex3f(bx+c*2, by, bz);
			break;
		}
		glEnd();
	}

	// Draw particles as small cubes
	const float hs = 1.5f; // half-size of cube
	for (int i = 0; i < sim->parts.active; i++)
	{
		if (!sim->parts[i].type) continue;
		int t = sim->parts[i].type;
		if (t <= 0 || t >= PT_NUM) continue;
		auto col = elements[t].Colour;
		glColor3ub(col.Red, col.Green, col.Blue);
		float cx = sim->parts[i].x;
		float cy = sim->parts[i].y;
		float cz = sim->parts[i].z;
		glBegin(GL_QUADS);
		// front (+Z)
		glVertex3f(cx-hs, cy-hs, cz+hs); glVertex3f(cx+hs, cy-hs, cz+hs);
		glVertex3f(cx+hs, cy+hs, cz+hs); glVertex3f(cx-hs, cy+hs, cz+hs);
		// back (-Z)
		glVertex3f(cx-hs, cy-hs, cz-hs); glVertex3f(cx-hs, cy+hs, cz-hs);
		glVertex3f(cx+hs, cy+hs, cz-hs); glVertex3f(cx+hs, cy-hs, cz-hs);
		// top (+Y)
		glVertex3f(cx-hs, cy+hs, cz-hs); glVertex3f(cx-hs, cy+hs, cz+hs);
		glVertex3f(cx+hs, cy+hs, cz+hs); glVertex3f(cx+hs, cy+hs, cz-hs);
		// bottom (-Y)
		glVertex3f(cx-hs, cy-hs, cz-hs); glVertex3f(cx+hs, cy-hs, cz-hs);
		glVertex3f(cx+hs, cy-hs, cz+hs); glVertex3f(cx-hs, cy-hs, cz+hs);
		// right (+X)
		glVertex3f(cx+hs, cy-hs, cz-hs); glVertex3f(cx+hs, cy+hs, cz-hs);
		glVertex3f(cx+hs, cy+hs, cz+hs); glVertex3f(cx+hs, cy-hs, cz+hs);
		// left (-X)
		glVertex3f(cx-hs, cy-hs, cz-hs); glVertex3f(cx-hs, cy-hs, cz+hs);
		glVertex3f(cx-hs, cy+hs, cz+hs); glVertex3f(cx-hs, cy+hs, cz-hs);
		glEnd();
	}

	SDL_GL_SwapWindow(g_win);
}

void CubeTest_ToggleCamControl() { g_camControl = !g_camControl; }

void CubeTest_Rotate(int dx, int dy)
{
	g_rotY += dx * 0.5f;
	g_rotX += dy * 0.5f;
}

static void ApplyViewMode(int mode)
{
	g_viewMode = mode;
	switch (mode) {
		case 0: g_rotX=0;   g_rotY=0;   break; // Front  �?face XY, +Z toward viewer
		case 1: g_rotX=-90; g_rotY=0;   break; // Top    �?face XZ, +Y toward viewer
		case 2: g_rotX=0;   g_rotY=90;  break; // Right  �?face YZ, +X toward viewer
		case 3: g_rotX=0;   g_rotY=180; break; // Back   �?face XY, -Z toward viewer
		case 4: g_rotX=90;  g_rotY=0;   break; // Bottom �?face XZ, -Y toward viewer
		case 5: g_rotX=0;   g_rotY=-90; break; // Left   �?face YZ, -X toward viewer
	}
}

void CubeTest_RotateView(int dir)
{
	// Transition table: viewMode × direction(0=Up 1=Down 2=Left 3=Right) → newViewMode
	static const int table[6][4] = {
		// Up   Down  Left  Right
		{ 1,   4,    5,    2 },  // Front (0)
		{ 3,   0,    5,    2 },  // Top   (1)
		{ 1,   4,    0,    3 },  // Right (2)
		{ 1,   4,    2,    5 },  // Back  (3)
		{ 0,   3,    5,    2 },  // Bottom(4)
		{ 1,   4,    3,    0 },  // Left  (5)
	};
	int newMode = table[g_viewMode][dir];
	ApplyViewMode(newMode);
}

void CubeTest_AdjustLayer(int delta)
{
	// Directly shift brush Z for persistent 3D positioning
	g_brushPZ += (float)delta;
}

void CubeTest_SetBrush(int x, int y, int rx, int ry)
{
	g_brushX = x; g_brushY = y;
	g_brushRX = rx; g_brushRY = ry;
}

void CubeTest_SetBrushPos(int x, int y)
{
	// TPT window: simulation area is at top-left (y=0..YRES-1),
	// menu bar (MENUSIZE=40) is at bottom. No Y offset needed.
	int sx = x, sy = y;
	if (sx < 0) sx = 0; if (sx >= XRES) sx = XRES - 1;
	if (sy < 0) sy = 0; if (sy >= YRES) sy = YRES - 1;
	g_brushX = sx; g_brushY = sy;
	// Only update the 2 active axes for current view; locked axis keeps its value
	switch (g_viewMode) {
		case 0: // Front: XY plane, Z locked. screen RIGHT=+X, DOWN=+Y
			g_brushPX = (float)sx;
			g_brushPY = (float)sy;
			break;
		case 3: // Back: XY plane, Z locked. screen RIGHT=-X, DOWN=+Y
			g_brushPX = (float)(XRES - 1 - sx);
			g_brushPY = (float)sy;
			break;
		case 1: // Top: XZ plane, Y locked. screen RIGHT=+X, UP=+Z
			g_brushPX = (float)sx;
			g_brushPZ = (float)(YRES - 1 - sy);
			break;
		case 4: // Bottom: XZ plane, Y locked. screen RIGHT=+X, DOWN=+Z
			g_brushPX = (float)sx;
			g_brushPZ = (float)sy;
			break;
		case 2: // Right: YZ plane, X locked. screen RIGHT=+Z, DOWN=+Y
			g_brushPY = (float)sy;
			g_brushPZ = (float)sx;
			break;
		case 5: // Left: YZ plane, X locked. screen RIGHT=-Z, DOWN=+Y
			g_brushPY = (float)sy;
			g_brushPZ = (float)(XRES - 1 - sx);
			break;
	}
}

void CubeTest_SetBrushRadius(int rx, int ry)
{
	g_brushRX = rx; g_brushRY = ry;
}

void CubeTest_ToggleZGrid()
{
	g_showZGrid = !g_showZGrid;
}

void CubeTest_ResetView()
{
	ApplyViewMode(0);
	g_dist = 800.0f;
}

void CubeTest_ToggleFullscreen()
{
	if (!g_win) return;
	Uint32 flags = SDL_GetWindowFlags(g_win);
	if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
		SDL_SetWindowFullscreen(g_win, 0);
	else
		SDL_SetWindowFullscreen(g_win, SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void CubeTest_Zoom(int delta)
{
	g_dist -= (float)delta;
	if (g_dist < 50) g_dist = 50;
	if (g_dist > 5000) g_dist = 5000;
}

int CubeTest_GetPlacementZ()
{
	// Brush Z is persistent across view switches
	return (int)(g_brushPZ + 0.5f);
}

void CubeTest_SetActiveTool(int toolType)
{
	g_activeToolType = toolType;
}

// Place particles along the 3D line from previous to current brush position
static float g_prevPX = 0, g_prevPY = 0, g_prevPZ = 0;
static bool g_havePrev = false;

static void PlaceParticleAtBrush()
{
	if (!g_sim || g_activeToolType <= 0) return;

	float cx = g_brushPX, cy = g_brushPY, cz = g_brushPZ;
	float dx = cx - g_prevPX, dy = cy - g_prevPY, dz = cz - g_prevPZ;
	auto *sim = const_cast<Simulation *>(g_sim);

	int steps = g_havePrev ? (int)ceilf(sqrtf(dx*dx + dy*dy + dz*dz)) : 0;
	if (steps <= 0)
	{
		int px = (int)(cx + 0.5f), py = (int)(cy + 0.5f), pz = (int)(cz + 0.5f);
		int i = sim->create_part(-2, px, py, g_activeToolType);
		if (i >= 0) sim->parts[i].z = (float)pz;
	}
	else
	{
		// 3D line interpolation, deduplicate same-pixel points
		int lastPX = -999, lastPY = -999;
		for (int s = 1; s <= steps; s++)
		{
			float t = (float)s / (float)steps;
			int px = (int)(g_prevPX + dx * t + 0.5f);
			int py = (int)(g_prevPY + dy * t + 0.5f);
			int pz = (int)(g_prevPZ + dz * t + 0.5f);
			if (px == lastPX && py == lastPY) continue;
			lastPX = px; lastPY = py;
			int i = sim->create_part(-2, px, py, g_activeToolType);
			if (i >= 0) sim->parts[i].z = (float)pz;
		}
	}

	g_prevPX = cx; g_prevPY = cy; g_prevPZ = cz;
	g_havePrev = true;
}

// Convert 3D window screen coords to world position on the active plane
static void UpdateBrushFrom3DWindow(int mx, int my)
{
	double wx, wy, wz;
	double rx, ry, rz; // ray direction

	// Unproject at near plane
	gluUnProject((double)mx, (double)(g_h - my), 0.0,
		g_modelview, g_proj, g_viewport, &wx, &wy, &wz);
	// Unproject at far plane
	gluUnProject((double)mx, (double)(g_h - my), 1.0,
		g_modelview, g_proj, g_viewport, &rx, &ry, &rz);
	rx -= wx; ry -= wy; rz -= wz; // ray direction

	// Intersect ray with the active plane (locked axis)
	double t;
	switch (g_viewMode) {
	case 0: case 3: // XY plane: Z locked at g_brushPZ
		if (fabs(rz) < 1e-9) return;
		t = ((double)g_brushPZ - wz) / rz;
		if (t < 0) return;
		g_brushPX = (float)(wx + rx * t);
		g_brushPY = (float)(wy + ry * t);
		if (g_brushPX < 0) g_brushPX = 0;
		if (g_brushPX >= XRES) g_brushPX = (float)(XRES - 1);
		if (g_brushPY < 0) g_brushPY = 0;
		if (g_brushPY >= YRES) g_brushPY = (float)(YRES - 1);
		break;
	case 1: case 4: // XZ plane: Y locked at g_brushPY
		if (fabs(ry) < 1e-9) return;
		t = ((double)g_brushPY - wy) / ry;
		if (t < 0) return;
		g_brushPX = (float)(wx + rx * t);
		g_brushPZ = (float)(wz + rz * t);
		if (g_brushPX < 0) g_brushPX = 0;
		if (g_brushPX >= XRES) g_brushPX = (float)(XRES - 1);
		if (g_brushPZ < 0) g_brushPZ = 0;
		if (g_brushPZ >= ZMAX) g_brushPZ = (float)(ZMAX - 1);
		break;
	default: // YZ plane (Right 2, Left 5): X locked at g_brushPX
		if (fabs(rx) < 1e-9) return;
		t = ((double)g_brushPX - wx) / rx;
		if (t < 0) return;
		g_brushPY = (float)(wy + ry * t);
		g_brushPZ = (float)(wz + rz * t);
		if (g_brushPY < 0) g_brushPY = 0;
		if (g_brushPY >= YRES) g_brushPY = (float)(YRES - 1);
		if (g_brushPZ < 0) g_brushPZ = 0;
		if (g_brushPZ >= ZMAX) g_brushPZ = (float)(ZMAX - 1);
		break;
	}
	// Sync g_brushX/Y for compatibility (used by brush render guard)
	g_brushX = (int)(g_brushPX + 0.5f);
	g_brushY = (int)(g_brushPY + 0.5f);
}

void CubeTest_HandleEvent(const SDL_Event &e)
{
	if (!g_win) return;
	auto wid = SDL_GetWindowID(g_win);
	if (e.type == SDL_WINDOWEVENT && e.window.windowID == wid)
	{
		if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		{ g_w = e.window.data1; g_h = e.window.data2; }
		else if (e.window.event == SDL_WINDOWEVENT_CLOSE)
		{ SDL_ShowCursor(SDL_ENABLE); CubeTest_Shutdown(); }
		else if (e.window.event == SDL_WINDOWEVENT_ENTER)
		{ SDL_ShowCursor(SDL_DISABLE); } // hide cursor over 3D window
		else if (e.window.event == SDL_WINDOWEVENT_LEAVE || e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
		{ SDL_ShowCursor(SDL_ENABLE);  } // show cursor when leaving
	}
	// 3D window mouse motion: camera drag if C held, else brush + continuous placement
	if (e.type == SDL_MOUSEMOTION && e.motion.windowID == wid)
	{
		static int lastMx3D = 0, lastMy3D = 0;
		if (g_camControl)
		{
			if (lastMx3D) CubeTest_Rotate(e.motion.x - lastMx3D, e.motion.y - lastMy3D);
			lastMx3D = e.motion.x; lastMy3D = e.motion.y;
		}
		else
		{
			lastMx3D = 0; lastMy3D = 0;
			UpdateBrushFrom3DWindow(e.motion.x, e.motion.y);
			if (g_placing) PlaceParticleAtBrush();
		}
	}
	// Left click in 3D window: place particle + start continuous placement
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && e.button.windowID == wid)
	{
		g_placing = true;
		PlaceParticleAtBrush();
	}
	// Left release: stop continuous placement, reset line interpolation
	if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
	{
		g_placing = false;
		g_havePrev = false;
	}
}

void CubeTest_Shutdown()
{
	if (g_gl) { SDL_GL_DeleteContext(g_gl); g_gl = nullptr; }
	if (g_win) { SDL_DestroyWindow(g_win); g_win = nullptr; }
}

bool CubeTest_IsOpen() { return g_win != nullptr; }

Uint32 CubeTest_GetWindowID()
{
	return g_win ? SDL_GetWindowID(g_win) : 0;
}
