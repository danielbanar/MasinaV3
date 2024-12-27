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
Controller::Controller(int i) : controllerIndex(i), nPacketNumber(0)
{
	SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
	if (SDL_NumJoysticks() < 1)
		std::cerr << "[Controller] No controller detected!\n";
	sdlController = SDL_JoystickOpen(controllerIndex);
	if (!sdlController)
		return;
	printf("[Controller] %s connected (%d)\t%d Axis %d Buttons\n", SDL_JoystickName(sdlController), controllerIndex, SDL_JoystickNumAxes(sdlController), SDL_JoystickNumButtons(sdlController));
}
Controller::~Controller()
{
	if (sdlController != nullptr)
		SDL_JoystickClose(sdlController);
	SDL_Quit();
}
void Controller::Poll()
{
	nPacketNumber++;

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
	}
	if (sdlController != nullptr)
	{
		static int num_axes = SDL_JoystickNumAxes(sdlController);
		static int num_buttons = SDL_JoystickNumButtons(sdlController);
		static bool lastButton = false;
		if (num_axes < 8 || num_buttons < 1)
			std::cerr << "[Controller] Unsupported controller - not enough axis/buttons (min 8/1)\n";
		for (int i = 0; i < num_axes; ++i)
			axis[i] = SDL_JoystickGetAxis(sdlController, i);
		for (int i = 0; i < num_buttons; ++i)
			buttons[i] = SDL_JoystickGetButton(sdlController, i);
		if(buttons[0] && !lastButton)
			failsafeMode = (FSMODE)(!failsafeMode);
		lastButton = buttons[0];
	}
	else
	{
		std::cerr << "[Controller] No controller detected!\n";
		sdlController = SDL_JoystickOpen(controllerIndex);
	}

}
std::string Controller::CreatePayload()
{
	uint16_t channel[10] = {
		nPacketNumber & 0xFFFF,
		mapRange(axis[0], -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(axis[1], -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(axis[2], -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(axis[3], -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),
		mapRange(axis[4], -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),//ARM
		mapRange(axis[5], -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),//FM
		mapRange(axis[7], -32768, 32767, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000),//FSMode - manual
		mapRange(axis[6], -32768, 32767, 0, 1),//Remote
		failsafeMode,//FSMode - disconnect
	};
	//("N(-?\\d+)RX(-?\\d+)RY(-?\\d+)LX(-?\\d+)LY(-?\\d+)SA(-?\\d+)SB(-?\\d+)SC(-?\\d+)SD(-?\\d+)SE(-?\\d+)CRC(-?\\d+)\\n");
	remote = channel[8];
	std::string result = "N" + std::to_string(nPacketNumber) +
		"RX" + std::to_string(channel[1]) +
		"RY" + std::to_string(channel[2]) +
		"LX" + std::to_string(channel[3]) +
		"LY" + std::to_string(channel[4]) +
		"SA" + std::to_string(channel[5]) +
		"SB" + std::to_string(channel[6]) +
		"SC" + std::to_string(channel[7]) +
		"REM" + std::to_string(channel[8]) +
		"FSM" + std::to_string(channel[9]) +
		"CRC" + std::to_string(CRC16(channel, 10)) + '\n';
	return result;
}

void Controller::Deadzone()
{

}

std::wstring Controller::GetFlags()
{
	return std::wstring(axis[4]>0 ? L"ARMED\n" : L"DISARMED\n") + (axis[7]>=0 ? L"FAILSAFE\n" : L"\n") + (failsafeMode ? L"LAND\n" : L"GPS\n") + (remote ? L"4G\n" : L"LOCAL\n");
}