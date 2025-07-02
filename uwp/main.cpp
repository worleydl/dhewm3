#include <Windows.h>
#include "SDL.h"

int CALLBACK WinMain(HINSTANCE h, HINSTANCE, LPSTR argv, int argc)
{
	return SDL_WinRTRunApp(SDL_main, NULL);
}
