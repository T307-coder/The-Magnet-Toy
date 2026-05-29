#include "Viewport3D.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include <SDL.h>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <cstdio>

Viewport3D::Viewport3D() = default;

Viewport3D::~Viewport3D()
{
	if (glContext) SDL_GL_DeleteContext(glContext);
	if (window) SDL_DestroyWindow(window);
}

bool Viewport3D::Init()
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

	window = SDL_CreateWindow("3D TPT Viewport",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		width, height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window)
	{
		fprintf(stderr, "3D Viewport: SDL_CreateWindow failed: %s\n", SDL_GetError());
		return false;
	}

	glContext = SDL_GL_CreateContext(window);
	if (!glContext)
	{
		fprintf(stderr, "3D Viewport: SDL_GL_CreateContext failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		window = nullptr;
		return false;
	}

	glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glPointSize(3.0f);
	return true;
}

void Viewport3D::Render(const Simulation *sim)
{
	if (!window) return;

	SDL_GL_MakeCurrent(window, glContext);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float aspect = (float)width / (float)height;
	gluPerspective(45.0, aspect, 10.0, 5000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Orbit camera
	float cx = camTargetX + camDist * cosf(camAngleY) * sinf(camAngleX);
	float cy = camTargetY + camDist * sinf(camAngleY);
	float cz = camTargetZ + camDist * cosf(camAngleY) * cosf(camAngleX);
	gluLookAt(cx, -cy, cz, camTargetX, -camTargetY, camTargetZ, 0, 1, 0);

	// Center the view on the 3D grid
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	glBegin(GL_POINTS);
	for (int i = 0; i < sim->parts.active; i++)
	{
		if (!sim->parts[i].type) continue;
		int type = sim->parts[i].type;
		float r = 1.0f, g = 1.0f, b = 1.0f;
		if (type > 0 && type < PT_NUM)
		{
			auto col = elements[type].Colour;
			r = col.Red / 255.0f;
			g = col.Green / 255.0f;
			b = col.Blue / 255.0f;
		}
		glColor3f(r, g, b);
		glVertex3f(sim->parts[i].x, -sim->parts[i].y, sim->parts[i].z);
	}
	glEnd();

	SDL_GL_SwapWindow(window);
}

void Viewport3D::HandleEvent(const SDL_Event &event)
{
	if (!window) return;

	if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(window))
	{
		if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		{
			width = event.window.data1;
			height = event.window.data2;
			glViewport(0, 0, width, height);
		}
		else if (event.window.event == SDL_WINDOWEVENT_CLOSE)
		{
			SDL_GL_DeleteContext(glContext);
			glContext = nullptr;
			SDL_DestroyWindow(window);
			window = nullptr;
		}
	}

	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		if (SDL_GetWindowID(window) == event.button.windowID)
		{
			mouseDown = true;
			lastMouseX = event.button.x;
			lastMouseY = event.button.y;
		}
	}
	if (event.type == SDL_MOUSEBUTTONUP)
	{
		mouseDown = false;
	}
	if (event.type == SDL_MOUSEMOTION && mouseDown)
	{
		if (SDL_GetWindowID(window) == event.motion.windowID)
		{
			camAngleX += (event.motion.x - lastMouseX) * 0.005f;
			camAngleY += (event.motion.y - lastMouseY) * 0.005f;
			if (camAngleY > 1.5f) camAngleY = 1.5f;
			if (camAngleY < -1.5f) camAngleY = -1.5f;
			lastMouseX = event.motion.x;
			lastMouseY = event.motion.y;
		}
	}
	if (event.type == SDL_MOUSEWHEEL)
	{
		camDist -= event.wheel.y * 20.0f;
		if (camDist < 50.0f) camDist = 50.0f;
		if (camDist > 2000.0f) camDist = 2000.0f;
	}
}
