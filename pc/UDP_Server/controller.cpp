#include "controller.h"
#define CRSF_CHANNEL_VALUE_MIN  172
#define CRSF_CHANNEL_VALUE_1000 191
#define CRSF_CHANNEL_VALUE_MID  992
#define CRSF_CHANNEL_VALUE_2000 1792
#define CRSF_CHANNEL_VALUE_MAX  1811
int mapRange(int input, int input_start, int input_end, int output_start, int output_end) {
	double proportion = (double)(input - input_start) / (double)(input_end - input_start);
	return output_start + (int)(proportion * (output_end - output_start) + 0.5);
}
int16_t CRC16(uint16_t* data, size_t length) {
	uint16_t crc = 0x0000; // Initial value
	uint16_t polynomial = 0x1021; // Polynomial for CRC-16-CCITT

	for (size_t i = 0; i < length; ++i) {
		uint16_t current_data = data[i];
		for (size_t j = 0; j < 16; ++j) {
			bool bit = (current_data >> (15 - j) & 1) ^ ((crc >> 15) & 1);
			crc <<= 1;
			if (bit) {
				crc ^= polynomial;
			}
		}
	}

	return crc;
}
Controller::Controller(int i) : controllerIndex(i), nPacketNumber(0), Buttons(0), LT(0), RT(0), LX(0), LY(0), RX(0), RY(0)
{
#ifdef _WIN32
	ZeroMemory(&state, sizeof(XINPUT_STATE));
#else
	SDL_Init(SDL_INIT_GAMECONTROLLER);
	sdlController = SDL_GameControllerOpen(controllerIndex);
#endif
}
Controller::~Controller()
{
#ifdef _WIN32

#else
	if (sdlController != nullptr)
		SDL_GameControllerClose(sdlController);

#endif
}
void Controller::Poll()
{
	nPacketNumber++;
#ifdef _WIN32
	DWORD dwResult = XInputGetState(controllerIndex, &state); // Get controller state
	static WORD prevButtons = 0;
	if (dwResult == ERROR_SUCCESS)
	{
		Buttons = state.Gamepad.wButtons;
		LT = state.Gamepad.bLeftTrigger;
		RT = state.Gamepad.bRightTrigger;
		LX = state.Gamepad.sThumbLX;
		LY = state.Gamepad.sThumbLY;
		RX = state.Gamepad.sThumbRX;
		RY = state.Gamepad.sThumbRY;
		if ((Buttons & XINPUT_GAMEPAD_BACK) && !(prevButtons & XINPUT_GAMEPAD_BACK))//ARM
			SA = !SA;
		if ((Buttons & XINPUT_GAMEPAD_DPAD_LEFT) && !(prevButtons & XINPUT_GAMEPAD_DPAD_LEFT))//FM
			SB = !SB;
		if ((Buttons & XINPUT_GAMEPAD_DPAD_RIGHT) && !(prevButtons & XINPUT_GAMEPAD_DPAD_RIGHT))//FM
			fsMode = !fsMode;
		if ((Buttons & XINPUT_GAMEPAD_DPAD_UP) && !(prevButtons & XINPUT_GAMEPAD_DPAD_UP))//FS
			fs = !fs;
		SC = fs ? ((uint8_t)fsMode + 1) : 0;
		if ((Buttons & XINPUT_GAMEPAD_START) && !(prevButtons & XINPUT_GAMEPAD_START))//LOCAL-REMOTE SWITCH
			remote = !remote;
		prevButtons = Buttons;
	}
	else
	{
		std::cerr << "Controller not connected." << std::endl;
	}
#else
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
	}
	if (sdlController != nullptr)
	{
		Buttons = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) * 256;
		LT = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 128;
		RT = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 128;
		LX = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTX);
		LY = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTY);
		RX = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_RIGHTX);
		RY = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_RIGHTY);
	}
	else
	{
		std::cerr << "Controller not connected." << std::endl;
		sdlController = SDL_GameControllerOpen(controllerIndex);
	}
#endif
}
std::string Controller::CreatePayload()
{
	uint16_t controls[10] = {
		nPacketNumber & 0xFFFF,
		mapRange(RX, -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(RY, -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(LY, -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(LX, -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(SA, 0, 1, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(SB, 0, 1, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(SC, 0, 2, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(SD, 0, 1, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(SE, 0, 1, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
	};
	//("N(-?\\d+)RX(-?\\d+)RY(-?\\d+)LX(-?\\d+)LY(-?\\d+)SA(-?\\d+)SB(-?\\d+)SC(-?\\d+)SD(-?\\d+)SE(-?\\d+)CRC(-?\\d+)\\n");
	uint8_t flags = 0;
	flags = (remote << 0) | (fsMode << 1);
	controls[9] += flags;
	std::string result = "N" + std::to_string(nPacketNumber) +
		"RX" + std::to_string(controls[1]) +
		"RY" + std::to_string(controls[2]) +
		"LX" + std::to_string(controls[3]) +
		"LY" + std::to_string(controls[4]) +
		"SA" + std::to_string(controls[5]) +
		"SB" + std::to_string(controls[6]) +
		"SC" + std::to_string(controls[7]) +
		"SD" + std::to_string(controls[8]) +
		"SE" + std::to_string(controls[9]) +
		"CRC" + std::to_string(CRC16(controls, 10)) + '\n';
	//std::cout << result;
	return result;
}

void Controller::Deadzone()
{
	if (LT < 10)
		LT = 0;
	if (RT < 10)
		RT = 0;
	if (-512 < LX && LX < 512)
		LX = 0;
	if (-512 < LY && LY < 512)
		LY = 0;
	if (-512 < RX && RX < 512)
		RX = 0;
	if (-512 < RY && RY < 512)
		RY = 0;

}
void Controller::Print()
{
	printf("%8ld LX: %8d LT: %8d RT: %8d\n", nPacketNumber, LX, LT, RT);
}

std::wstring Controller::GetFlags()
{
	return std::wstring(SA ? L"ARMED\n" : L"DISARMED\n") + (fs ? L"FAILSAFE\n" : L"\n") + (fsMode ? L"LAND\n" : L"GPS\n") + (remote ? L"4G\n" : L"LOCAL\n");
}