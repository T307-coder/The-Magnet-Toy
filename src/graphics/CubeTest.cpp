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
static int g_brushShape = 0;  // 0=cube, 1=sphere
static float g_brushRX = 8.0f, g_brushRY = 8.0f, g_brushRZ = 8.0f; // per-axis radii
static bool g_showZGrid = false;
// View mode and full 3D brush position
static int g_viewMode = 0;
static float g_brushPX = 0, g_brushPY = 0, g_brushPZ = 0;
static int g_activeToolType = 0; // current tool element type for 3D clicks
static bool g_placing = false;   // left button held → continuous placement
static bool g_deleting = false;   // right button held → continuous deletion
static bool g_shiftHeld = false;  // Shift → X-axis scroll
static bool g_altHeld = false;    // Alt → Y-axis scroll
static bool g_xHeld = false;      // X key → Z-axis scroll
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

	// ---- 3D Brush preview: wireframe cube or sphere rings ----
	if (g_brushX >= 0 && g_brushY >= 0)
	{
		float bx = g_brushPX, by = g_brushPY, bz = g_brushPZ;
		float rx = g_brushRX, ry = g_brushRY, rz = g_brushRZ;
		glColor3f(1.0f, 1.0f, 0.3f);
		glBegin(GL_LINES);

		if (g_brushShape == 0) // Cube wireframe
		{
			// 12 edges with per-axis half-extents
			glVertex3f(bx-rx, by-ry, bz-rz); glVertex3f(bx+rx, by-ry, bz-rz);
			glVertex3f(bx-rx, by+ry, bz-rz); glVertex3f(bx+rx, by+ry, bz-rz);
			glVertex3f(bx-rx, by-ry, bz+rz); glVertex3f(bx+rx, by-ry, bz+rz);
			glVertex3f(bx-rx, by+ry, bz+rz); glVertex3f(bx+rx, by+ry, bz+rz);
			glVertex3f(bx-rx, by-ry, bz-rz); glVertex3f(bx-rx, by+ry, bz-rz);
			glVertex3f(bx+rx, by-ry, bz-rz); glVertex3f(bx+rx, by+ry, bz-rz);
			glVertex3f(bx-rx, by-ry, bz+rz); glVertex3f(bx-rx, by+ry, bz+rz);
			glVertex3f(bx+rx, by-ry, bz+rz); glVertex3f(bx+rx, by+ry, bz+rz);
			glVertex3f(bx-rx, by-ry, bz-rz); glVertex3f(bx-rx, by-ry, bz+rz);
			glVertex3f(bx+rx, by-ry, bz-rz); glVertex3f(bx+rx, by-ry, bz+rz);
			glVertex3f(bx-rx, by+ry, bz-rz); glVertex3f(bx-rx, by+ry, bz+rz);
			glVertex3f(bx+rx, by+ry, bz-rz); glVertex3f(bx+rx, by+ry, bz+rz);
		}
		else // Sphere/ellipsoid: 3 great circles
		{
			const int N = 48;
			for (int i = 0; i < N; i++) {
				float a0 = (float)i / N * 6.283185f;
				float a1 = (float)(i+1) / N * 6.283185f;
				glVertex3f(bx+cosf(a0)*rx, by+sinf(a0)*ry, bz);
				glVertex3f(bx+cosf(a1)*rx, by+sinf(a1)*ry, bz);
				glVertex3f(bx+cosf(a0)*rx, by, bz+sinf(a0)*rz);
				glVertex3f(bx+cosf(a1)*rx, by, bz+sinf(a1)*rz);
				glVertex3f(bx, by+cosf(a0)*ry, bz+sinf(a0)*rz);
				glVertex3f(bx, by+cosf(a1)*ry, bz+sinf(a1)*rz);
			}
		}
		glEnd();
	}

	// Draw particles as small cubes

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

	// ---- Brush coordinate overlay (screen-space) ----
#ifdef _WIN32
	if (g_fontBase && g_brushX >= 0)
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, g_w, g_h, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);

		char buf[64];
		snprintf(buf, sizeof(buf), "X:%.0f Y:%.0f Z:%.0f  R:%d S:%s",
			g_brushPX, g_brushPY, g_brushPZ,
			(int)(g_brushRX + 0.5f),
			g_brushShape == 0 ? "Cube" : "Sphere");
		glColor3f(1, 1, 0.6f);
		glRasterPos2i(10, 20);
		glListBase(g_fontBase);
		glCallLists((GLsizei)strlen(buf), GL_UNSIGNED_BYTE, buf);

		glEnable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	// ---- 3 cross-section slice views (top-right corner) ----
	if (sim)
	{
		const int S = 150; // slice size in pixels
		const int margin = 10;
		float scale = (float)S / (float)XRES; // XRES=YRES=ZMAX=384

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, g_w, g_h, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);

		auto &sd2 = SimulationData::CRef();
		auto &el2 = sd2.elements;

		for (int sliceIdx = 0; sliceIdx < 3; sliceIdx++)
		{
			int ox = g_w - S - margin;
			int oy = margin + sliceIdx * (S + margin);
			float lockVal; // locked axis value for this slice
			const char *label;

			if (sliceIdx == 0) { lockVal = g_brushPZ; label = "XY (Z locked)"; }
			else if (sliceIdx == 1) { lockVal = g_brushPY; label = "XZ (Y locked)"; }
			else { lockVal = g_brushPX; label = "YZ (X locked)"; }

			// Background
			glColor3f(0.05f, 0.05f, 0.08f);
			glBegin(GL_QUADS);
			glVertex2i(ox, oy); glVertex2i(ox+S, oy);
			glVertex2i(ox+S, oy+S); glVertex2i(ox, oy+S);
			glEnd();

			// Particles within ±1 unit of the slice plane
			glPointSize(2.0f);
			glBegin(GL_POINTS);
			for (int i = 0; i < sim->parts.active; i++)
			{
				if (!sim->parts[i].type) continue;
				int t = sim->parts[i].type;
				if (t <= 0 || t >= PT_NUM) continue;
				auto col = el2[t].Colour;
				glColor3ub(col.Red, col.Green, col.Blue);

				float px = sim->parts[i].x, py = sim->parts[i].y, pz = sim->parts[i].z;
				int sx, sy;

				if (sliceIdx == 0) { // XY slice
					if (fabsf(pz - lockVal) > 1.5f) continue;
					sx = ox + (int)(px * scale);
					sy = oy + (int)(py * scale);
				} else if (sliceIdx == 1) { // XZ slice
					if (fabsf(py - lockVal) > 1.5f) continue;
					sx = ox + (int)(px * scale);
					sy = oy + (int)((ZMAX - 1 - pz) * scale);
				} else { // YZ slice
					if (fabsf(px - lockVal) > 1.5f) continue;
					sx = ox + (int)(pz * scale);
					sy = oy + (int)(py * scale);
				}
				glVertex2i(sx, sy);
			}
			glEnd();
			glPointSize(1.0f);

			// Brush crosshair on slice
			int bx, by;
			if (sliceIdx == 0) { bx = ox + (int)(g_brushPX * scale); by = oy + (int)(g_brushPY * scale); }
			else if (sliceIdx == 1) { bx = ox + (int)(g_brushPX * scale); by = oy + (int)((ZMAX - 1 - g_brushPZ) * scale); }
			else { bx = ox + (int)(g_brushPZ * scale); by = oy + (int)(g_brushPY * scale); }

			int cs = 5;
			glColor3f(1, 1, 0);
			glBegin(GL_LINES);
			glVertex2i(bx-cs, by); glVertex2i(bx+cs, by);
			glVertex2i(bx, by-cs); glVertex2i(bx, by+cs);
			glEnd();

			// Border
			glColor3f(0.3f, 0.3f, 0.4f);
			glBegin(GL_LINE_LOOP);
			glVertex2i(ox, oy); glVertex2i(ox+S, oy);
			glVertex2i(ox+S, oy+S); glVertex2i(ox, oy+S);
			glEnd();
		}

		glEnable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
#endif

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
	// 3D brush now has independent per-axis radii; ignore 2D sync
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

void CubeTest_ResizeBrush(int delta, int axis)
{
	auto clamp = [](float &v) { if (v < 0) v = 0; if (v > 100) v = 100; };
	if (axis == 0)      { g_brushRX += (float)delta; g_brushRY += (float)delta; g_brushRZ += (float)delta; }
	else if (axis == 1) { g_brushRX += (float)delta; }
	else if (axis == 2) { g_brushRY += (float)delta; }
	else if (axis == 3) { g_brushRZ += (float)delta; }
	clamp(g_brushRX); clamp(g_brushRY); clamp(g_brushRZ);
}

void CubeTest_ToggleBrushShape()
{
	g_brushShape = (g_brushShape + 1) % 2;
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

// Create a 3D particle bypassing pmap — allows multiple particles per (x,y)
static int CreatePart3D(Simulation *sim, int x, int y, int z, int t)
{
	if (x<0 || y<0 || x>=XRES || y>=YRES) return -1;
	if (t<=0 || t>=PT_NUM) return -1;
	auto &sd = SimulationData::CRef();
	if (!sd.elements[t].Enabled) return -1;

	if (sim->parts.active >= NPART) return -1;
	int i = sim->parts.active++;
	auto &p = sim->parts[i];
	p = sd.elements[t].DefaultProperties;
	p.type = t;
	p.x = (float)x;
	p.y = (float)y;
	p.z = (float)z;
	p.tmp5 = 0;
	p.tmp6 = 0;

	if (sd.elements[t].Create)
		(*(sd.elements[t].Create))(sim, i, x, y, t, -1);

	sim->elementCount[t]++;
	return i;
}

// Unified brush action: mode 0=place, 1=delete
static void BrushAction(int cx, int cy, int cz, int mode)
{
	auto *sim = const_cast<Simulation *>(g_sim);
	int rx = (int)(g_brushRX + 0.5f);
	int ry = (int)(g_brushRY + 0.5f);
	int rz = (int)(g_brushRZ + 0.5f);
	if (rx < 0) rx = 0; if (ry < 0) ry = 0; if (rz < 0) rz = 0;

	if (mode == 0) // Place
	{
		if (g_brushShape == 0) // Box
		{
			for (int dx = -rx; dx <= rx; dx++)
				for (int dy = -ry; dy <= ry; dy++)
					for (int dz = -rz; dz <= rz; dz++)
						CreatePart3D(sim, cx + dx, cy + dy, cz + dz, g_activeToolType);
		}
		else // Ellipsoid
		{
			float irx2 = rx>0 ? 1.0f/((float)rx*rx) : 1e9f;
			float iry2 = ry>0 ? 1.0f/((float)ry*ry) : 1e9f;
			float irz2 = rz>0 ? 1.0f/((float)rz*rz) : 1e9f;
			for (int dx = -rx; dx <= rx; dx++)
				for (int dy = -ry; dy <= ry; dy++)
					for (int dz = -rz; dz <= rz; dz++)
					{
						if ((float)(dx*dx)*irx2 + (float)(dy*dy)*iry2 + (float)(dz*dz)*irz2 > 1.0f) continue;
						CreatePart3D(sim, cx + dx, cy + dy, cz + dz, g_activeToolType);
					}
		}
	}
	else // Delete
	{
		float irx2 = rx>0 ? 1.0f/((float)rx*rx) : 1e9f;
		float iry2 = ry>0 ? 1.0f/((float)ry*ry) : 1e9f;
		float irz2 = rz>0 ? 1.0f/((float)rz*rz) : 1e9f;
		for (int i = 0; i < sim->parts.active; i++)
		{
			if (!sim->parts[i].type) continue;
			int dx = (int)(sim->parts[i].x + 0.5f) - cx;
			int dy = (int)(sim->parts[i].y + 0.5f) - cy;
			int dz = (int)(sim->parts[i].z + 0.5f) - cz;

			bool hit;
			if (g_brushShape == 0)
				hit = (abs(dx) <= rx && abs(dy) <= ry && abs(dz) <= rz);
			else
				hit = ((float)(dx*dx)*irx2 + (float)(dy*dy)*iry2 + (float)(dz*dz)*irz2 <= 1.0f);

			if (hit) sim->kill_part(i);
		}
	}
}

// Place particles along the 3D line from previous to current brush position
static float g_prevPX = 0, g_prevPY = 0, g_prevPZ = 0;
static bool g_havePrev = false;

static void PlaceParticleAtBrush()
{
	if (!g_sim || g_activeToolType <= 0) return;

	float cx = g_brushPX, cy = g_brushPY, cz = g_brushPZ;
	float dx = cx - g_prevPX, dy = cy - g_prevPY, dz = cz - g_prevPZ;

	int steps = g_havePrev ? (int)ceilf(sqrtf(dx*dx + dy*dy + dz*dz) * 2.0f) : 0;
	if (steps <= 0)
	{
		BrushAction((int)(cx + 0.5f), (int)(cy + 0.5f), (int)(cz + 0.5f), 0);
	}
	else
	{
		for (int s = 1; s <= steps; s++)
		{
			float t = (float)s / (float)steps;
			int px = (int)(g_prevPX + dx * t + 0.5f);
			int py = (int)(g_prevPY + dy * t + 0.5f);
			int pz = (int)(g_prevPZ + dz * t + 0.5f);
			BrushAction(px, py, pz, 0);
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
	// 3D window mouse motion: camera drag if C held, else brush + continuous placement/deletion
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
			if (g_deleting) BrushAction(
				(int)(g_brushPX + 0.5f), (int)(g_brushPY + 0.5f), (int)(g_brushPZ + 0.5f), 1);
		}
	}
	// Left click: place; Right click: delete
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.windowID == wid)
	{
		if (e.button.button == SDL_BUTTON_LEFT)
		{
			g_placing = true;
			PlaceParticleAtBrush();
		}
		else if (e.button.button == SDL_BUTTON_RIGHT)
		{
			g_deleting = true;
			BrushAction((int)(g_brushPX + 0.5f), (int)(g_brushPY + 0.5f), (int)(g_brushPZ + 0.5f), 1);
		}
	}
	// Release: stop placing or deleting
	if (e.type == SDL_MOUSEBUTTONUP)
	{
		if (e.button.button == SDL_BUTTON_LEFT)
		{
			g_placing = false;
			g_havePrev = false;
		}
		else if (e.button.button == SDL_BUTTON_RIGHT)
		{
			g_deleting = false;
		}
	}
	// Scroll in 3D window: uniform or per-axis resize
	if (e.type == SDL_MOUSEWHEEL && e.wheel.windowID == wid)
	{
		int d = e.wheel.y;
		if (SDL_GetModState() & KMOD_CTRL)
			CubeTest_Zoom(d * 30);
		else if (g_xHeld)
			CubeTest_ResizeBrush(d, 3); // Z axis
		else if (g_shiftHeld)
			CubeTest_ResizeBrush(d, 1); // X axis
		else if (g_altHeld)
			CubeTest_ResizeBrush(d, 2); // Y axis
		else
			CubeTest_ResizeBrush(d, 0); // uniform
	}
	// Track modifier keys (any window — CubeTest sees all events)
	if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
	{
		bool down = (e.type == SDL_KEYDOWN);
		auto sc = e.key.keysym.scancode;
		if (sc == SDL_SCANCODE_LSHIFT || sc == SDL_SCANCODE_RSHIFT) g_shiftHeld = down;
		if (sc == SDL_SCANCODE_LALT || sc == SDL_SCANCODE_RALT)     g_altHeld = down;
		if (sc == SDL_SCANCODE_X)                                   g_xHeld = down;
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

int CubeTest_Get2DViewMode()
{
	// Map 3D view to 2D plane: Front/Back=XY, Top/Bottom=XZ, Right/Left=YZ
	if (g_viewMode == 0 || g_viewMode == 3) return 0; // XY
	if (g_viewMode == 1 || g_viewMode == 4) return 1; // XZ
	return 2; // YZ
}

float CubeTest_Get2DLockedVal()
{
	int mode = CubeTest_Get2DViewMode();
	if (mode == 0) return g_brushPZ; // XY: Z locked
	if (mode == 1) return g_brushPY; // XZ: Y locked
	return g_brushPX; // YZ: X locked
}

// ============ 3 orthogonal 2D slice windows ============
struct SliceWin {
	SDL_Window *win = nullptr;
	SDL_GLContext gl = nullptr;
	int w = 400, h = 400;
};
static SliceWin g_slice[3];
static const char *g_sliceTitle[3] = {
	"TPT Slice: XY (Z=brush)",
	"TPT Slice: XZ (Y=brush)",
	"TPT Slice: YZ (X=brush)"
};

bool SliceWindow_Init(SlicePlane plane)
{
#ifdef _WIN32
	auto &s = g_slice[plane];
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	s.win = SDL_CreateWindow(g_sliceTitle[plane],
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		s.w, s.h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!s.win) return false;
	s.gl = SDL_GL_CreateContext(s.win);
	return s.gl != nullptr;
#else
	return false;
#endif
}

void SliceWindow_Render(SlicePlane plane)
{
#ifdef _WIN32
	auto &s = g_slice[plane];
	if (!s.win || !s.gl || !g_sim) return;
	SDL_GL_MakeCurrent(s.win, s.gl);
	glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, s.w, s.h, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	auto &sd = SimulationData::CRef();
	auto &el = sd.elements;

	float lockVal;
	if (plane == SLICE_XY) lockVal = g_brushPZ;
	else if (plane == SLICE_XZ) lockVal = g_brushPY;
	else lockVal = g_brushPX;

	float scale = (float)s.w / (float)XRES; // square 384×384 grid

	// Particles near slice plane
	glPointSize(2.5f);
	glBegin(GL_POINTS);
	for (int i = 0; i < g_sim->parts.active; i++)
	{
		if (!g_sim->parts[i].type) continue;
		int t = g_sim->parts[i].type;
		if (t <= 0 || t >= PT_NUM) continue;
		auto col = el[t].Colour;
		glColor3ub(col.Red, col.Green, col.Blue);

		float px = g_sim->parts[i].x, py = g_sim->parts[i].y, pz = g_sim->parts[i].z;
		int sx, sy;
		if (plane == SLICE_XY) {
			if (fabsf(pz - lockVal) > 1.5f) continue;
			sx = (int)(px * scale); sy = (int)(py * scale);
		} else if (plane == SLICE_XZ) {
			if (fabsf(py - lockVal) > 1.5f) continue;
			sx = (int)(px * scale); sy = (int)((ZMAX - 1 - pz) * scale);
		} else {
			if (fabsf(px - lockVal) > 1.5f) continue;
			sx = (int)(pz * scale); sy = (int)(py * scale);
		}
		glVertex2i(sx, sy);
	}
	glEnd();
	glPointSize(1.0f);

	// Brush crosshair
	int bx, by;
	if (plane == SLICE_XY) {
		bx = (int)(g_brushPX * scale); by = (int)(g_brushPY * scale);
	} else if (plane == SLICE_XZ) {
		bx = (int)(g_brushPX * scale); by = (int)((ZMAX - 1 - g_brushPZ) * scale);
	} else {
		bx = (int)(g_brushPZ * scale); by = (int)(g_brushPY * scale);
	}
	int cs = 8;
	glColor3f(1, 1, 0);
	glBegin(GL_LINES);
	glVertex2i(bx-cs, by); glVertex2i(bx+cs, by);
	glVertex2i(bx, by-cs); glVertex2i(bx, by+cs);
	glEnd();

	// Title bar text
	char title[64];
	snprintf(title, sizeof(title), "%s  Z:%.0f", g_sliceTitle[plane],
		plane == SLICE_XY ? g_brushPZ : plane == SLICE_XZ ? g_brushPY : g_brushPX);
	SDL_SetWindowTitle(s.win, title);

	SDL_GL_SwapWindow(s.win);
#endif
}

void SliceWindow_HandleEvent(SlicePlane plane, const SDL_Event &e)
{
	auto &s = g_slice[plane];
	if (!s.win) return;
	auto wid = SDL_GetWindowID(s.win);
	if (e.type == SDL_WINDOWEVENT && e.window.windowID == wid)
	{
		if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		{ s.w = e.window.data1; s.h = e.window.data2; }
		else if (e.window.event == SDL_WINDOWEVENT_CLOSE)
		{ SliceWindow_Shutdown(plane); }
	}
}

void SliceWindow_Shutdown(SlicePlane plane)
{
	auto &s = g_slice[plane];
	if (s.gl) { SDL_GL_DeleteContext(s.gl); s.gl = nullptr; }
	if (s.win) { SDL_DestroyWindow(s.win); s.win = nullptr; }
}

bool SliceWindow_IsOpen(SlicePlane plane) { return g_slice[plane].win != nullptr; }

Uint32 SliceWindow_GetID(SlicePlane plane)
{
	return g_slice[plane].win ? SDL_GetWindowID(g_slice[plane].win) : 0;
}
