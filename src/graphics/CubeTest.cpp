// SDL2+OpenGL 3D particle viewport
#include "CubeTest.h"
#include "SimulationConfig.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "gui/game/GameController.h"
#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <cmath>
#include <cstdio>
#include <vector>
#include <memory>

extern SDL_Window *sdl_window; // main TPT 2D window (PowderToySDL.cpp)

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
static bool g_showZGrid = true; // Z-grid on by default for spatial reference
// View mode and full 3D brush position
static int g_viewMode = 0;
static float g_brushPX = 0, g_brushPY = 0, g_brushPZ = 0;
static int g_activeToolType = 0; // current tool element type for 3D clicks
static bool g_placing = false;   // left button held → continuous placement
static bool g_deleting = false;   // right button held → continuous deletion
static bool g_shiftHeld = false;  // Shift → X-axis scroll / line mode
static bool g_altHeld = false;    // Alt → Y-axis scroll
static bool g_xHeld = false;      // X key → Z-axis scroll
static const int ZMAX = 384;     // Z extent (matches YRES for cubic volume)

// FPS counter
static int g_frameCount = 0;
static Uint32 g_lastFpsTime = 0;
static float g_currentFps = 0.0f;

// Tool modes for 3D drawing (matching 2D tool concepts)
static bool g_lineMode = false;   // Shift+left/right: 3D line
static bool g_rectMode = false;   // Ctrl+left/right: 3D cuboid
static bool g_toolDelete = false; // true = delete mode, false = place mode
static float g_toolStartX, g_toolStartY, g_toolStartZ; // anchor point for line/rect
// Cached matrices for gluUnProject
static double g_proj[16], g_modelview[16];
static int    g_viewport[4];

// First-person free camera (V toggles)
static bool g_freeCam = false;
static float g_camPX = 192.0f, g_camPY = 192.0f, g_camPZ = 192.0f; // center of volume
static float g_camYaw = 0.0f, g_camPitch = 0.0f; // radians, yaw=0 → +Z, pitch=0 → horizontal
static float g_brushDist = 60.0f; // brush distance from camera
static bool g_camKeyFwd = false, g_camKeyBack = false, g_camKeyLeft = false, g_camKeyRight = false;
static bool g_camKeyUp = false, g_camKeyDown = false; // pgup/pgdn

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

	// Free cam: apply held-key movement every frame
	if (g_freeCam)
	{
		float speed = 1.0f;
		float fwdX = sinf(g_camYaw), fwdZ = cosf(g_camYaw);
		float rgtX = cosf(g_camYaw), rgtZ = -sinf(g_camYaw);
		if (g_camKeyFwd)  { g_camPX += fwdX * speed; g_camPZ += fwdZ * speed; }
		if (g_camKeyBack) { g_camPX -= fwdX * speed; g_camPZ -= fwdZ * speed; }
		if (g_camKeyLeft) { g_camPX += rgtX * speed; g_camPZ += rgtZ * speed; }
		if (g_camKeyRight){ g_camPX -= rgtX * speed; g_camPZ -= rgtZ * speed; }
		if (g_camKeyUp)   { g_camPY -= speed; }
		if (g_camKeyDown) { g_camPY += speed; }
		if (g_camPX < 0) g_camPX = 0; if (g_camPX >= XRES) g_camPX = XRES-1;
		if (g_camPY < 0) g_camPY = 0; if (g_camPY >= YRES) g_camPY = YRES-1;
		if (g_camPZ < 0) g_camPZ = 0; if (g_camPZ >= ZMAX) g_camPZ = ZMAX-1;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, g_w, g_h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float aspect = (float)g_w / (float)g_h;
	if (g_freeCam)
		gluPerspective(60.0, aspect, 1.0, 5000.0); // wider FOV for first-person
	else
		gluPerspective(45.0, aspect, 10.0, 10000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (g_freeCam)
	{
		// First-person camera: TPT coords → Y-flipped OpenGL space
		float lx = cosf(g_camPitch) * sinf(g_camYaw);
		float ly = -sinf(g_camPitch); // positive pitch → look upward (decrease TPT Y)
		float lz = cosf(g_camPitch) * cosf(g_camYaw);

		// gluLookAt with Y-flipped coords (must match glScalef below)
		// Camera looks from (cx,-cy,cz) toward (cx+lx, -(cy+ly), cz+lz)
		gluLookAt(g_camPX, -g_camPY, g_camPZ,
		          g_camPX + lx, -(g_camPY + ly), g_camPZ + lz,
		          0, 1, 0);
		// Y-flip for OpenGL — applied BEFORE look-at in vertex pipeline
		glScalef(1, -1, 1);

		// Brush position in TPT coords (clamped to volume)
		g_brushPX = g_camPX + lx * g_brushDist;
		g_brushPY = g_camPY + ly * g_brushDist;
		g_brushPZ = g_camPZ + lz * g_brushDist;
		if (g_brushPX < 0) g_brushPX = 0; if (g_brushPX >= XRES) g_brushPX = XRES-1;
		if (g_brushPY < 0) g_brushPY = 0; if (g_brushPY >= YRES) g_brushPY = YRES-1;
		if (g_brushPZ < 0) g_brushPZ = 0; if (g_brushPZ >= ZMAX) g_brushPZ = ZMAX-1;
	}
	else
	{
		glTranslatef(0, 0, -g_dist);
		glRotatef(g_rotX, 1, 0, 0);
		glRotatef(g_rotY, 0, 1, 0);
		// Pivot to center the 3D volume (X:0..XRES, Y:0..YRES, Z:0..ZMAX)
		// Y flipped so TPT Y=0 is at screen top
		glTranslatef(-XRES / 2.0f, YRES / 2.0f, -ZMAX / 2.0f);
		glScalef(1, -1, 1);
	}

	// Cache matrices for gluUnProject (3D window mouse → world coords)
	glGetDoublev(GL_PROJECTION_MATRIX, g_proj);
	glGetDoublev(GL_MODELVIEW_MATRIX, g_modelview);
	glGetIntegerv(GL_VIEWPORT, g_viewport);

	// ---- Lighting: two directional lights + ambient for 6 distinct face shades ----
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_NORMALIZE);
	// Ambient: base visibility for all faces
	GLfloat globalAmb[] = { 0.10f, 0.10f, 0.12f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
	// Light 0: upper-right-front (brightens top, right, front)
	GLfloat l0Pos[]  = { 0.5f, 0.7f, 0.5f, 0.0f };
	GLfloat l0Dif[]  = { 0.60f, 0.60f, 0.55f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  l0Dif);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  globalAmb); // reuse
	// Light 1: lower-left-back (brightens bottom, left, back)
	GLfloat l1Pos[]  = { -0.4f, -0.9f, -0.3f, 0.0f };
	GLfloat l1Dif[]  = { 0.35f, 0.38f, 0.40f, 1.0f };
	glLightfv(GL_LIGHT1, GL_POSITION, l1Pos);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  l1Dif);
	glLightfv(GL_LIGHT1, GL_AMBIENT,  globalAmb);

	// ---- Three orthogonal grid planes (XY floor, XZ back, YZ left) ----
	glDisable(GL_LIGHTING); // grids and lines don't need lighting
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

	// ---- 3D Line / Rect preview ----
	if (g_lineMode || g_rectMode)
	{
		float ex = g_brushPX, ey = g_brushPY, ez = g_brushPZ;
		float sx = g_toolStartX, sy = g_toolStartY, sz = g_toolStartZ;
		glBegin(GL_LINES);
		bool del = g_toolDelete;
		if (g_lineMode) // Line preview: magenta (place) / red (delete)
		{
			glColor3f(del ? 1.0f : 1.0f, del ? 0.2f : 0.3f, del ? 0.2f : 1.0f);
			glVertex3f(sx, sy, sz);
			glVertex3f(ex, ey, ez);
		}
		else // Cuboid wireframe: cyan (place) / red (delete)
		{
			glColor3f(del ? 1.0f : 0.3f, del ? 0.2f : 1.0f, del ? 0.2f : 1.0f);
			// 12 edges from (sx,sy,sz) to (ex,ey,ez)
			glVertex3f(sx,sy,sz); glVertex3f(ex,sy,sz);
			glVertex3f(sx,ey,sz); glVertex3f(ex,ey,sz);
			glVertex3f(sx,sy,ez); glVertex3f(ex,sy,ez);
			glVertex3f(sx,ey,ez); glVertex3f(ex,ey,ez);
			glVertex3f(sx,sy,sz); glVertex3f(sx,ey,sz);
			glVertex3f(ex,sy,sz); glVertex3f(ex,ey,sz);
			glVertex3f(sx,sy,ez); glVertex3f(sx,ey,ez);
			glVertex3f(ex,sy,ez); glVertex3f(ex,ey,ez);
			glVertex3f(sx,sy,sz); glVertex3f(sx,sy,ez);
			glVertex3f(ex,sy,sz); glVertex3f(ex,sy,ez);
			glVertex3f(sx,ey,sz); glVertex3f(sx,ey,ez);
			glVertex3f(ex,ey,sz); glVertex3f(ex,ey,ez);
		}
		glEnd();
	}

	// ---- 3D Brush preview: wireframe cube or sphere rings ----
	if (g_brushX >= 0 && g_brushY >= 0 && !g_lineMode && !g_rectMode)
	{
		float bx = g_brushPX, by = g_brushPY, bz = g_brushPZ;
		float rx = g_brushRX, ry = g_brushRY, rz = g_brushRZ;
		bool delPreview = g_deleting;
		glColor3f(delPreview ? 1.0f : 1.0f, delPreview ? 0.3f : 1.0f, 0.3f);
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
	glEnable(GL_LIGHTING);

	const float hs = 0.5f; // half-size: 1-unit cubes
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
		glNormal3f(0, 0, 1);
		glVertex3f(cx-hs, cy-hs, cz+hs); glVertex3f(cx+hs, cy-hs, cz+hs);
		glVertex3f(cx+hs, cy+hs, cz+hs); glVertex3f(cx-hs, cy+hs, cz+hs);
		// back (-Z)
		glNormal3f(0, 0, -1);
		glVertex3f(cx-hs, cy-hs, cz-hs); glVertex3f(cx-hs, cy+hs, cz-hs);
		glVertex3f(cx+hs, cy+hs, cz-hs); glVertex3f(cx+hs, cy-hs, cz-hs);
		// top (+Y)
		glNormal3f(0, 1, 0);
		glVertex3f(cx-hs, cy+hs, cz-hs); glVertex3f(cx-hs, cy+hs, cz+hs);
		glVertex3f(cx+hs, cy+hs, cz+hs); glVertex3f(cx+hs, cy+hs, cz-hs);
		// bottom (-Y)
		glNormal3f(0, -1, 0);
		glVertex3f(cx-hs, cy-hs, cz-hs); glVertex3f(cx+hs, cy-hs, cz-hs);
		glVertex3f(cx+hs, cy-hs, cz+hs); glVertex3f(cx-hs, cy-hs, cz+hs);
		// right (+X)
		glNormal3f(1, 0, 0);
		glVertex3f(cx+hs, cy-hs, cz-hs); glVertex3f(cx+hs, cy+hs, cz-hs);
		glVertex3f(cx+hs, cy+hs, cz+hs); glVertex3f(cx+hs, cy-hs, cz+hs);
		// left (-X)
		glNormal3f(-1, 0, 0);
		glVertex3f(cx-hs, cy-hs, cz-hs); glVertex3f(cx-hs, cy-hs, cz+hs);
		glVertex3f(cx-hs, cy+hs, cz+hs); glVertex3f(cx-hs, cy+hs, cz-hs);
		glEnd();
	}

	// ---- Brush coordinate overlay (screen-space) ----
	glDisable(GL_LIGHTING);
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

	// ---- FPS display ----
	g_frameCount++;
	Uint32 now = SDL_GetTicks();
	if (now - g_lastFpsTime >= 1000)
	{
		g_currentFps = g_frameCount * 1000.0f / (now - g_lastFpsTime);
		g_frameCount = 0;
		g_lastFpsTime = now;
	}
	if (g_fontBase && g_currentFps > 0)
	{
		char fpsBuf[32];
		snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.0f", g_currentFps);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, g_w, g_h, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glColor3f(0.0f, 1.0f, 0.0f);
		glRasterPos2i(10, 30);
		glListBase(g_fontBase);
		glCallLists((GLsizei)strlen(fpsBuf), GL_UNSIGNED_BYTE, fpsBuf);
		glEnable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
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
	// Adjust the locked axis based on current view plane (clamped to volume bounds)
	int v2d = CubeTest_Get2DViewMode();
	if (v2d == 0)      { g_brushPZ += (float)delta; if (g_brushPZ < 0) g_brushPZ = 0; if (g_brushPZ >= ZMAX) g_brushPZ = ZMAX-1; }
	else if (v2d == 1) { g_brushPY += (float)delta; if (g_brushPY < 0) g_brushPY = 0; if (g_brushPY >= YRES) g_brushPY = YRES-1; }
	else               { g_brushPX += (float)delta; if (g_brushPX < 0) g_brushPX = 0; if (g_brushPX >= XRES) g_brushPX = XRES-1; }
}

void CubeTest_SetBrush(int x, int y, int rx, int ry)
{
	g_brushX = x; g_brushY = y;
	g_brushRX = rx; g_brushRY = ry;
}

void CubeTest_SetBrushPos(int x, int y)
{
	// In free cam mode, brush is controlled by camera, not 2D mouse
	if (g_freeCam) return;

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

void CubeTest_RaiseWindow()
{
	if (g_win) SDL_RaiseWindow(g_win);
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
// Forward decls
static int CreatePart3D(Simulation *sim, int x, int y, int z, int t);
static void BrushAction(int cx, int cy, int cz, int mode);

// 3D flood fill: fill all connected empty space from (sx,sy,sz)
static void FloodFill3D(int sx, int sy, int sz)
{
	if (!g_sim || g_activeToolType <= 0) return;
	auto *sim = const_cast<Simulation *>(g_sim);
	if (sx<0||sy<0||sz<0||sx>=XRES||sy>=YRES||sz>=ZMAX) return;

	// Check if start position is already occupied by checking pmap
	if (sim->pmap[sy][sx]) return; // occupied in 2D grid

	// Simple BFS flood fill in 3D
	// Use a vector as queue — conservative size estimate
	std::vector<int> qx, qy, qz;
	qx.reserve(1024); qy.reserve(1024); qz.reserve(1024);
	qx.push_back(sx); qy.push_back(sy); qz.push_back(sz);

	// Bitmap for visited (2D only since pmap is 2D)
	auto bitmap = std::make_unique<char[]>(XRES * YRES);
	std::fill(bitmap.get(), bitmap.get() + XRES * YRES, 0);

	int count = 0;
	while (!qx.empty())
	{
		int x = qx.back(); qx.pop_back();
		int y = qy.back(); qy.pop_back();
		int z = qz.back(); qz.pop_back();

		if (x<0||y<0||z<0||x>=XRES||y>=YRES||z>=ZMAX) continue;
		int idx = y * XRES + x;
		if (bitmap[idx]) continue;
		if (sim->pmap[y][x]) continue; // occupied in 2D
		bitmap[idx] = 1;

		CreatePart3D(sim, x, y, z, g_activeToolType);
		if (++count > 20000) break; // safety limit

		// 6-directional neighbors
		qx.push_back(x+1); qy.push_back(y);   qz.push_back(z);
		qx.push_back(x-1); qy.push_back(y);   qz.push_back(z);
		qx.push_back(x);   qy.push_back(y+1); qz.push_back(z);
		qx.push_back(x);   qy.push_back(y-1); qz.push_back(z);
		qx.push_back(x);   qy.push_back(y);   qz.push_back(z+1);
		qx.push_back(x);   qy.push_back(y);   qz.push_back(z-1);
	}
}

// Draw 3D line with brush shape from (sx,sy,sz) to (ex,ey,ez)
// mode: 0=place, 1=delete
static void DrawLine3D(int sx, int sy, int sz, int ex, int ey, int ez, int mode = 0)
{
	if (!g_sim) return;
	if (mode == 0 && g_activeToolType <= 0) return;
	float dx = (float)(ex - sx), dy = (float)(ey - sy), dz = (float)(ez - sz);
	int steps = (int)ceilf(sqrtf(dx*dx + dy*dy + dz*dz));
	if (steps <= 0) { BrushAction(sx, sy, sz, mode); return; }

	for (int s = 0; s <= steps; s++)
	{
		float t = (float)s / (float)steps;
		int px = (int)((float)sx + dx * t + 0.5f);
		int py = (int)((float)sy + dy * t + 0.5f);
		int pz = (int)((float)sz + dz * t + 0.5f);
		BrushAction(px, py, pz, mode);
	}
}

// 3D flood delete: BFS following connected same-type particles and deleting them
static void FloodDelete3D(int sx, int sy, int sz)
{
	if (!g_sim) return;
	auto *sim = const_cast<Simulation *>(g_sim);
	if (sx<0||sy<0||sz<0||sx>=XRES||sy>=YRES||sz>=ZMAX) return;

	// Find target type at start position
	int targetType = 0;
	int targetIdx = -1;
	for (int i = 0; i < sim->parts.active; i++)
	{
		if (!sim->parts[i].type) continue;
		int px = (int)(sim->parts[i].x + 0.5f);
		int py = (int)(sim->parts[i].y + 0.5f);
		int pz = (int)(sim->parts[i].z + 0.5f);
		if (px == sx && py == sy && pz == sz) { targetType = sim->parts[i].type; targetIdx = i; break; }
	}
	if (targetType <= 0) return; // nothing to delete

	// BFS following same-type particles (6-directional)
	std::vector<int> qx, qy, qz;
	qx.reserve(1024); qy.reserve(1024); qz.reserve(1024);
	qx.push_back(sx); qy.push_back(sy); qz.push_back(sz);

	// Use a simple visited tracking: we mark deleted particles by type=0
	// and use kill_part which also removes from pmap.
	// But we need to avoid infinite loops, so track visited positions.
	// Since we're deleting as we go, we just need to find neighbors first.
	sim->kill_part(targetIdx); // delete the seed

	int count = 1;
	while (!qx.empty())
	{
		int cx = qx.back(); qx.pop_back();
		int cy = qy.back(); qy.pop_back();
		int cz = qz.back(); qz.pop_back();

		// Check 6 neighbors
		static const int dirs[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
		for (int d = 0; d < 6; d++)
		{
			int nx = cx + dirs[d][0], ny = cy + dirs[d][1], nz = cz + dirs[d][2];
			if (nx<0||ny<0||nz<0||nx>=XRES||ny>=YRES||nz>=ZMAX) continue;

			for (int i = 0; i < sim->parts.active; i++)
			{
				if (!sim->parts[i].type || sim->parts[i].type != targetType) continue;
				int px = (int)(sim->parts[i].x + 0.5f);
				int py = (int)(sim->parts[i].y + 0.5f);
				int pz = (int)(sim->parts[i].z + 0.5f);
				if (px == nx && py == ny && pz == nz)
				{
					qx.push_back(nx); qy.push_back(ny); qz.push_back(nz);
					sim->kill_part(i);
					if (++count > 50000) { qx.clear(); break; } // safety limit
					break;
				}
			}
			if (count > 50000) break;
		}
	}
}

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
	// Helper: take history snapshot before first mutation in a gesture
	auto snap = [&]() {
		if (!g_placing && !g_deleting && !g_lineMode && !g_rectMode)
			GameController::Ref().HistorySnapshot();
	};
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
	// 3D window mouse motion: camera drag if C held, else brush/tool
	if (e.type == SDL_MOUSEMOTION && e.motion.windowID == wid)
	{
		static int lastMx3D = 0, lastMy3D = 0;
		if (g_freeCam)
		{
			// Free cam: mouse rotates view (yaw/pitch)
			float dx = (float)e.motion.xrel;
			float dy = (float)e.motion.yrel;
			g_camYaw   -= dx * 0.004f;
			g_camPitch -= dy * 0.004f;
			// Clamp pitch to avoid flipping
			const float maxPitch = 3.14159265f * 0.49f;
			if (g_camPitch >  maxPitch) g_camPitch =  maxPitch;
			if (g_camPitch < -maxPitch) g_camPitch = -maxPitch;
			// Keep yaw in [0, 2π)
			while (g_camYaw < 0) g_camYaw += 6.2831853f;
			while (g_camYaw >= 6.2831853f) g_camYaw -= 6.2831853f;

			// Continuous placement/deletion while mouse moves
			if (g_lineMode || g_rectMode)
				{ /* preview updates via render */ }
			else if (g_placing)
				PlaceParticleAtBrush();
			else if (g_deleting)
				BrushAction((int)(g_brushPX+0.5f),(int)(g_brushPY+0.5f),(int)(g_brushPZ+0.5f),1);
		}
		else if (g_camControl)
		{
			if (lastMx3D) CubeTest_Rotate(e.motion.x - lastMx3D, e.motion.y - lastMy3D);
			lastMx3D = e.motion.x; lastMy3D = e.motion.y;
		}
		else
		{
			lastMx3D = 0; lastMy3D = 0;
			UpdateBrushFrom3DWindow(e.motion.x, e.motion.y);
			if (g_lineMode || g_rectMode)
				{ /* just update preview via render */ }
			else if (g_placing)
				PlaceParticleAtBrush();
			else if (g_deleting)
				BrushAction((int)(g_brushPX+0.5f),(int)(g_brushPY+0.5f),(int)(g_brushPZ+0.5f),1);
		}
	}
	// Left click: depending on modifiers
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && e.button.windowID == wid)
	{
		if ((SDL_GetModState() & KMOD_CTRL) && (SDL_GetModState() & KMOD_SHIFT))
		{
			// Ctrl+Shift: 3D flood fill
			snap();
			FloodFill3D((int)(g_brushPX+0.5f), (int)(g_brushPY+0.5f), (int)(g_brushPZ+0.5f));
		}
		else if (SDL_GetModState() & KMOD_CTRL)
		{
			// Ctrl: rect mode
			g_rectMode = true;
			g_toolDelete = false;
			g_toolStartX = g_brushPX; g_toolStartY = g_brushPY; g_toolStartZ = g_brushPZ;
		}
		else if (g_shiftHeld)
		{
			// Shift: line mode
			g_lineMode = true;
			g_toolDelete = false;
			g_toolStartX = g_brushPX; g_toolStartY = g_brushPY; g_toolStartZ = g_brushPZ;
		}
		else
		{
			snap();
			g_placing = true;
			PlaceParticleAtBrush();
		}
	}
	// Right click: delete — with modifier support for line/rect/fill
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT && e.button.windowID == wid)
	{
		if ((SDL_GetModState() & KMOD_CTRL) && (SDL_GetModState() & KMOD_SHIFT))
		{
			// Ctrl+Shift+Right: 3D flood delete (same-type connected)
			snap();
			FloodDelete3D((int)(g_brushPX+0.5f), (int)(g_brushPY+0.5f), (int)(g_brushPZ+0.5f));
		}
		else if (SDL_GetModState() & KMOD_CTRL)
		{
			// Ctrl+Right: rect delete mode
			g_rectMode = true;
			g_toolDelete = true;
			g_toolStartX = g_brushPX; g_toolStartY = g_brushPY; g_toolStartZ = g_brushPZ;
		}
		else if (g_shiftHeld)
		{
			// Shift+Right: line delete mode
			g_lineMode = true;
			g_toolDelete = true;
			g_toolStartX = g_brushPX; g_toolStartY = g_brushPY; g_toolStartZ = g_brushPZ;
		}
		else
		{
			snap();
			g_deleting = true;
			BrushAction((int)(g_brushPX+0.5f),(int)(g_brushPY+0.5f),(int)(g_brushPZ+0.5f),1);
		}
	}
	// Release: execute line/rect or stop brush
	if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
	{
		if (g_lineMode)
		{
			g_lineMode = false;
			DrawLine3D((int)(g_toolStartX+0.5f),(int)(g_toolStartY+0.5f),(int)(g_toolStartZ+0.5f),
			           (int)(g_brushPX+0.5f),(int)(g_brushPY+0.5f),(int)(g_brushPZ+0.5f), 0);
		}
		else if (g_rectMode)
		{
			g_rectMode = false;
			// Fill cuboid from start to current brush
			int sx = (int)(g_toolStartX+0.5f), sy = (int)(g_toolStartY+0.5f), sz = (int)(g_toolStartZ+0.5f);
			int ex = (int)(g_brushPX+0.5f), ey = (int)(g_brushPY+0.5f), ez = (int)(g_brushPZ+0.5f);
			int x1 = sx<ex ? sx : ex, x2 = sx>ex ? sx : ex;
			int y1 = sy<ey ? sy : ey, y2 = sy>ey ? sy : ey;
			int z1 = sz<ez ? sz : ez, z2 = sz>ez ? sz : ez;
			auto *sim = const_cast<Simulation *>(g_sim);
			int mode = g_toolDelete ? 1 : 0;
			if (mode == 0) {
				for (int x=x1; x<=x2; x++)
					for (int y=y1; y<=y2; y++)
						for (int z=z1; z<=z2; z++)
							CreatePart3D(sim, x, y, z, g_activeToolType);
			} else {
				for (int x=x1; x<=x2; x++)
					for (int y=y1; y<=y2; y++)
						for (int z=z1; z<=z2; z++)
							BrushAction(x, y, z, 1);
			}
			g_toolDelete = false;
		}
		else
		{
			g_placing = false;
			g_havePrev = false;
		}
	}
	if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_RIGHT)
	{
		if (g_lineMode)
		{
			g_lineMode = false;
			DrawLine3D((int)(g_toolStartX+0.5f),(int)(g_toolStartY+0.5f),(int)(g_toolStartZ+0.5f),
			           (int)(g_brushPX+0.5f),(int)(g_brushPY+0.5f),(int)(g_brushPZ+0.5f), 1);
			g_toolDelete = false;
		}
		else if (g_rectMode)
		{
			g_rectMode = false;
			int sx = (int)(g_toolStartX+0.5f), sy = (int)(g_toolStartY+0.5f), sz = (int)(g_toolStartZ+0.5f);
			int ex = (int)(g_brushPX+0.5f), ey = (int)(g_brushPY+0.5f), ez = (int)(g_brushPZ+0.5f);
			int x1 = sx<ex ? sx : ex, x2 = sx>ex ? sx : ex;
			int y1 = sy<ey ? sy : ey, y2 = sy>ey ? sy : ey;
			int z1 = sz<ez ? sz : ez, z2 = sz>ez ? sz : ez;
			for (int x=x1; x<=x2; x++)
				for (int y=y1; y<=y2; y++)
					for (int z=z1; z<=z2; z++)
						BrushAction(x, y, z, 1);
			g_toolDelete = false;
		}
		else
		{
			g_deleting = false;
		}
	}
	// Scroll in 3D window: uniform or per-axis resize
	if (e.type == SDL_MOUSEWHEEL && e.wheel.windowID == wid)
	{
		int d = e.wheel.y;
		if (g_freeCam && (SDL_GetModState() & KMOD_CTRL))
		{
			// Free cam: Ctrl+scroll adjusts brush distance
			g_brushDist += (float)d * 5.0f;
			if (g_brushDist < 10.0f)  g_brushDist = 10.0f;
			if (g_brushDist > 500.0f) g_brushDist = 500.0f;
		}
		else if (SDL_GetModState() & KMOD_CTRL)
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

		// 3D window keyboard shortcuts (only when 3D window has focus)
		if (e.key.windowID == wid)
		{
			if (down && sc == SDL_SCANCODE_F)
			{
				GameController::Ref().FrameStep(); // F = single frame step (same as 2D)
			}
			if (down && sc == SDL_SCANCODE_GRAVE) // Backtick ` = toggle focus to 2D window
			{
				if (sdl_window) SDL_RaiseWindow(sdl_window);
			}
		}

		// Free cam movement keys + V toggle
		if (down && sc == SDL_SCANCODE_V) { CubeTest_ToggleFreeCam(); }
		if (g_freeCam)
		{
			if (sc == SDL_SCANCODE_UP)       g_camKeyFwd  = down;
			if (sc == SDL_SCANCODE_DOWN)     g_camKeyBack = down;
			if (sc == SDL_SCANCODE_LEFT)     g_camKeyLeft = down;
			if (sc == SDL_SCANCODE_RIGHT)    g_camKeyRight = down;
			if (sc == SDL_SCANCODE_PAGEUP)   g_camKeyUp   = down;
			if (sc == SDL_SCANCODE_PAGEDOWN) g_camKeyDown = down;
		}
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

bool CubeTest_IsFreeCam() { return g_freeCam; }

void CubeTest_ToggleFreeCam()
{
	g_freeCam = !g_freeCam;
	if (g_freeCam)
	{
		g_camPX = XRES / 2.0f;
		g_camPY = YRES / 2.0f;
		g_camPZ = ZMAX / 2.0f;
		g_camYaw = 0.0f;
		g_camPitch = 0.0f;
		g_brushDist = 60.0f;
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	else
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}
