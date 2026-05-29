#pragma once

struct SDL_Window;
union SDL_Event;
class Simulation;

class Viewport3D
{
	SDL_Window *window = nullptr;
	void *glContext = nullptr;
	int width = 640, height = 480;
	float camAngleX = 0.5f, camAngleY = 0.3f;
	float camDist = 400.0f;
	float camTargetX = 0, camTargetY = 0, camTargetZ = 0;
	bool mouseDown = false;
	int lastMouseX = 0, lastMouseY = 0;

public:
	Viewport3D();
	~Viewport3D();
	bool Init();
	void Render(const Simulation *sim);
	void HandleEvent(const SDL_Event &event);
	bool IsOpen() const { return window != nullptr; }
};
