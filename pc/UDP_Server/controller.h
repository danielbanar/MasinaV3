#pragma once
#include <iostream>
#include <string>
#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

enum FSMODE : uint8_t
{
	GPS,
	LAND
};
class Controller
{
public:
	Controller(int i);
	~Controller();
	void Poll();
	void Print();
	void Deadzone();
	std::string CreatePayload();
	std::wstring GetFlags();
private:
	SDL_Joystick* sdlController;
	int controllerIndex;
	unsigned long nPacketNumber;
	int16_t axis[8] = {0};
	uint8_t buttons[24] = {0};
	bool remote = false;
	FSMODE failsafeMode = GPS;
	bool failsafe = false;
};