#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <ctime>
#include <locale>
#include <string>
#include <mutex>
#include <regex>
#include "controller.h"
#include "server.h"
#include <iostream>
#include <string>
#include <fcntl.h>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <cwchar>
#include <time.h>
#include <mutex>
#include <cmath>
#include <string>
#include <locale>
#include <codecvt>
#define NO_CONTROLLER_

std::wstring convert_to_wstring(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

struct Telemetry
{
	uint16_t voltage, current;
	uint32_t capacity;
	uint8_t remaining;

	int32_t latitude, longitude;
	uint16_t groundspeed, heading, altitude;
	uint8_t satellites;

	uint16_t verticalspd;

	std::string flightMode;

	int16_t pitch, roll;
	uint16_t yaw;

	uint8_t rxRssiPercent, rxRfPower;

	uint8_t txRssiPercent, txRfPower, txFps;

	int16_t rssi, rsrq, rsrp;
	int8_t snr;

	uint8_t uplink_RSSI_1;
	uint8_t uplink_RSSI_2;
	uint8_t uplink_Link_quality;
	int8_t uplink_SNR;
	uint8_t active_antenna;
	uint8_t rf_Mode;
	uint8_t uplink_TX_Power;
	uint8_t downlink_RSSI;
	uint8_t downlink_Link_quality;
	int8_t downlink_SNR;

	int pi_temp;
	int pi_read_speed;
	int pi_write_speed;
	int pi_rssi;
	int pi_snr;
};
Telemetry tel = { 0 };
std::mutex sharedMutex;
int serCells = 4; // 4S default
//Domo
double homeLat = 48.4145948, homeLon = 17.6957299;

//Pole
//double homeLat = 48.413256, homeLon = 17.692330;
//Soporna
double pinLat = 48.246096, pinLon = 17.817368;
#ifndef NO_CONTROLLER
Controller controller(0);
#endif
void ProcessPayload(std::vector<uint8_t> payload)
{
	if (payload.size() == 0)
		return;
	std::lock_guard<std::mutex> lock(sharedMutex);
	if (payload[0] == 0x08) // CRSF_FRAMETYPE_BATTERY_SENSOR
	{
		tel.voltage = (payload[1] << 8) | payload[2];
		tel.current = (payload[3] << 8) | payload[4];
		tel.capacity = (payload[5] << 16) | (payload[6] << 8) | payload[7];
		tel.remaining = payload[8];
#ifdef _DEBUG
		printf("BATTERY_SENSOR: %.1fV\t%.1fA\t%dmAh\t%d%%\n", ((float)tel.voltage) / 10, ((float)tel.current) / 10, tel.capacity, tel.remaining);
#endif // DEBUG

	}
	else if (payload[0] == 0x02) // CRSF_FRAMETYPE_GPS
	{
		tel.latitude = (payload[1] << 24) | (payload[2] << 16) | (payload[3] << 8) | payload[4];  // degree / 10,000,000 big endian
		tel.longitude = (payload[5] << 24) | (payload[6] << 16) | (payload[7] << 8) | payload[8]; // degree / 10,000,000 big endian
		tel.groundspeed = (payload[9] << 8) | payload[10];                                        // km/h / 10 big endian
		tel.heading = (payload[11] << 8) | payload[12];                                           // GPS heading, degree/100 big endian
		tel.altitude = ((payload[13] << 8) | payload[14]) - 1000;                                 // meters, +1000m big endian
		tel.satellites = payload[15];                                                             // satellites
#ifdef _DEBUG
		printf("GPS: %lf, %lf\tGspd: %dkm/h\tHdg: %d°\tAlt: %dm\tSat: %d\n", ((double)tel.latitude) / 10000000.0, ((double)tel.longitude) / 10000000.0, tel.groundspeed / 10, tel.heading / 100, tel.altitude, tel.satellites);
#endif // DEBUG
		printf("GPS: %lf, %lf\tGspd: %dkm/h\tHdg: %d°\tAlt: %dm\tSat: %d\n", ((double)tel.latitude) / 10000000.0, ((double)tel.longitude) / 10000000.0, tel.groundspeed / 10, tel.heading / 100, tel.altitude, tel.satellites);
	}
	else if (payload[0] == 0x07) // CRSF_FRAMETYPE_VARIO
	{
		tel.verticalspd = (payload[1] << 8) | payload[2]; // Vertical speed in cm/s, BigEndian
#ifdef _DEBUG
		printf("VSpd: %dcm/s\n", tel.verticalspd);
#endif // DEBUG

	}
	else if (payload[0] == 0x21) // CRSF_FRAMETYPE_FLIGHT_MODE
	{
		char flightMode[32] = { 0 };
		memcpy(flightMode, &payload[1], payload.size() - 1);
		tel.flightMode = flightMode;
#ifdef _DEBUG
		printf("FM: %s\n", flightMode);
#endif // DEBUG

	}
	else if (payload[0] == 0x1E) // CRSF_FRAMETYPE_ATTITUDE
	{
		tel.pitch = (payload[1] << 8) | payload[2]; // pitch in radians, BigEndian
		tel.roll = (payload[3] << 8) | payload[4];  // roll in radians, BigEndian
		tel.yaw = (payload[5] << 8) | payload[6];   // yaw in radians, BigEndian
#ifdef _DEBUG
		printf("Gyro:\tP%.1f\tR%.1f\tY%.1f\n", RADTODEG(tel.pitch) / 10000.f, RADTODEG(tel.roll) / 10000.f, RADTODEG(tel.yaw) / 10000.f);
#endif // DEBUG
	}
	else if (payload[0] == 0x1C) // CRSF_FRAMETYPE_LINK_RX_ID
	{
		tel.rxRssiPercent = payload[1];
		tel.rxRfPower = payload[2]; // should be signed int?
#ifdef _DEBUG
		printf("RX: RSSI: %d\tPWR:\t%d\n", tel.rxRssiPercent, tel.rxRfPower);
#endif // DEBUG
	}
	else if (payload[0] == 0x1D) // CRSF_FRAMETYPE_LINK_RX_ID
	{
		tel.txRssiPercent = payload[1];
		tel.txRfPower = payload[2]; // should be signed int?
		tel.txFps = payload[3];
#ifdef _DEBUG
		printf("TX: RSSI: %d\tPWR: %d\tFps: %d\n", tel.txRssiPercent, tel.txRfPower, tel.txFps);
#endif // DEBUG
	}
	else if (payload[0] == 0x88) // CRSF_FRAMETYPE_LINK_RX_ID
	{
		for (size_t i = 0; i < 8; i++)
		{
			std::cout << payload[i];
		}

		tel.rssi = (payload[2] << 8) | payload[1];
		tel.rsrq = (payload[4] << 8) | payload[3];
		tel.rsrp = (payload[5] << 8) | payload[6];
		tel.rsrp = payload[7];
#ifdef _DEBUG
		printf("RSSI: %d\tRSQR: %d\tRSRP: %d\tSNR: %d\n", tel.rssi, tel.rsrq, tel.rsrp, tel.rsrp);
#endif // DEBUG
	}
	else if (payload[0] == 0x14) // CRSF_FRAMETYPE_LINK_RX_ID
	{
		tel.uplink_RSSI_1 = payload[1];
		tel.uplink_RSSI_2 = payload[2];
		tel.uplink_Link_quality = payload[3];
		tel.uplink_SNR = payload[4];
		tel.active_antenna = payload[5];
		tel.rf_Mode = payload[6];
		tel.uplink_TX_Power = payload[7];
		tel.downlink_RSSI = payload[8];
		tel.downlink_Link_quality = payload[9];
		tel.downlink_SNR = payload[10];
#ifdef _DEBUG
		printf("RSSI1: %d\tRSSI2: %d\tRQly: %d\tRSNR: %d\tAnt: %d\tRF_MD: %d\tTXP: %d\tTRSSI: %d\tTQly: %d\tTSNR: %d\n", tel.uplink_RSSI_1, tel.uplink_RSSI_2, tel.uplink_Link_quality, tel.uplink_SNR, tel.active_antenna, tel.rf_Mode, tel.uplink_TX_Power, tel.downlink_RSSI, tel.downlink_Link_quality, tel.downlink_SNR);
#endif // DEBUG
	}
}


std::wstring Compass(int angle)
{
	int scale = 10;
	angle += 180 + std::round((double)scale / 2.0);
	if (angle >= 360)
		angle -= 360;
	if (angle >= 360 || angle < 0)
		return L"";
	angle /= scale;
	wchar_t buffer[37] = { 0 }; // size should be (360 / scale) + 1
	wmemset(buffer, L'-', 360 / scale);
	buffer[0] = L'N';
	buffer[90 / scale] = L'E';
	buffer[180 / scale] = L'S';
	buffer[270 / scale] = L'W';

	int homeIndex = (int)(calculateAngle((double)tel.latitude / 10000000.0, (double)tel.longitude / 10000000.0, homeLat, homeLon) / scale);
	//std::wstring homeSymbol = L"🏠";
	//wcsncpy(buffer + homeIndex, homeSymbol.c_str(), homeSymbol.size());
	buffer[homeIndex] = L'H';
	buffer[(int)(calculateAngle((double)tel.latitude / 10000000.0, (double)tel.longitude / 10000000.0, pinLat, pinLon) / scale)] = L'J';
	std::wstring s(buffer);

	return s.substr(angle + 1) + s.substr(0, angle);
}


void CheckPayloads(std::vector<uint8_t>& buffer)
{
	while (true)
	{
		size_t start = -1;
		// Scan for Start byte
		for (int i = 0; i < buffer.size(); i++)
		{
			if (buffer[i] == 0xC8)
			{
				start = i;
				break;
			}
		}
		// if Start byte not found return and read more from serial
		if (start == -1)
			return;
		/*..SYNC .?*/
		// Trim to start byte
		buffer.erase(buffer.begin(), buffer.begin() + start);
		/*SYNC .?*/
		// Check for payload size - aka anti out of bounds - aka sync byte is not last byte in buffer
		if (buffer.size() < 2)
			return;
		/*SYNC LEN .?*/
		size_t payload_length = buffer[1];
		// Check if entire payload is in buffer / aka anti out of bounds
		if (buffer.size() < payload_length + 2)
			return;
		// YES we have entire payload in buffer
		/*SYNC 04 01 02 03 04 CRC*/
		if (payload_length > 60)
		{
			buffer.clear();
			std::cerr << "invalid crsf packet detected! - payload length > 60\n";
			return;
		}
		//printf("CRC: %02X\n", CRC(buffer,2,payload_length-1));
		if (CRC(buffer, 2, payload_length - 1) != buffer[payload_length + 1])
		{
			buffer.clear();
			std::cerr << "invalid crsf packet detected! - CRC mismatch\n";
			return;
		}
		//more sanity checks
		if (buffer.size() < payload_length + 3)
			return;

		/*SYNC 04 01 02 03 04 CRC SYNC*/
		if (buffer[payload_length + 2] != 0xC8)
		{
			buffer.clear();
			std::cerr << "invalid crsf packet detected!\n";
			return;
		}
#ifdef _DEBUG
		printVectorHex(buffer, payload_length + 2);
#endif
		// Process it and remove from buffer

		ProcessPayload((std::vector<uint8_t>(buffer.begin() + 2, buffer.begin() + payload_length + 2)));
		buffer.erase(buffer.begin(), buffer.begin() + payload_length + 2);
	}
}

#define RADTODEG(radians) ((radians) * (180.0 / M_PI))
#define DEGTORAD(degrees) ((degrees) * (M_PI / 180.0))

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "D2d1.lib")
#pragma comment(lib, "Dwrite.lib")

#define CHAR_TO_WCHAR(str) \
    ([](const char* input) { \
        int length = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0); \
        wchar_t* output = new wchar_t[length]; \
        MultiByteToWideChar(CP_UTF8, 0, input, -1, output, length); \
        return output; \
    })(str)

const char* TARGET_WINDOW_NAME = "Direct3D11 renderer";
char rxbuffer1[1024] = { 0 };
char rxbuffer2[1024] = { 0 };
ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pRenderTarget = NULL;
IDWriteFactory* pDWriteFactory = NULL;
IDWriteTextFormat* pTextFormat = NULL;
ID2D1SolidColorBrush* pBrush = NULL;
ID2D1SolidColorBrush* pOutlineBrush = NULL;

void InitD2D(HWND hwnd);
void CleanupD2D();
void RenderText(HWND hwnd);
void Update(HWND hwnd, HWND targetWnd);
int initializeSocket(int port, struct sockaddr_in& serverAddr);

int main(int argc, char* argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-home") == 0 && i + 2 < argc)
		{
			homeLat = atof(argv[i + 1]);
			homeLon = atof(argv[i + 2]);
			i += 2;
		}
		else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)
		{
			serCells = atoi(argv[i + 1]);
			++i;
		}
	}
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
		return EXIT_FAILURE;
	}

	int serverAddrLen = sizeof(sockaddr_in);
	struct sockaddr_in serverAddr1, serverAddr2;

	int serverSocket1 = initializeSocket(2223, serverAddr1);
	if (serverSocket1 == INVALID_SOCKET) {
		WSACleanup();
		return EXIT_FAILURE;
	}

	int serverSocket2 = initializeSocket(2224, serverAddr2);
	if (serverSocket2 == INVALID_SOCKET) {
		closesocket(serverSocket1);
		WSACleanup();
		return EXIT_FAILURE;
	}

	const char* CLASS_NAME = "OverlayWindowClass";
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // Transparent background

	RegisterClass(&wc);
	HWND hwnd = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
		CLASS_NAME,
		"Overlay",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hwnd) {
		std::cerr << "[Overlay] Failed to create overlay window";
		return 1;
	}

	SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY); // Make the window transparent

	ShowWindow(hwnd, SW_SHOW);

	InitD2D(hwnd);

	HWND targetWnd = FindWindow(NULL, TARGET_WINDOW_NAME);
	if (!targetWnd) {
		std::cerr << "[Overlay] Target window not found!\n";
	}
	std::thread traccarThread(TraccarUpdate);

	static std::vector<uint8_t> buffer;
	while (true)
	{
#ifndef NO_CONTROLLER
		controller.Poll();
		controller.Deadzone();
#endif
		sockaddr_in clientAddr1;
		sockaddr_in clientAddr2;
		{
			int clientAddrLen1 = sizeof(clientAddr1);

#ifndef NO_CONTROLLER
			std::string messageToSend = controller.CreatePayload();
#else
			std::string messageToSend = "CRSF\n";
#endif

			memset(rxbuffer1, 0, sizeof(rxbuffer1));
			int bytesRead = recvfrom(serverSocket1, (char*)rxbuffer1, sizeof(rxbuffer1), 0, (struct sockaddr*)&clientAddr1, &clientAddrLen1);
			if (bytesRead == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
				{
					LPSTR errorMessage = nullptr;
					FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMessage, 0, NULL);
					std::cerr << "Error receiving data from UDP socket: " << errorMessage;
				}
			}
			else
			{
				clientAddr2.sin_addr = clientAddr1.sin_addr;
				buffer.insert(buffer.end(), &rxbuffer1[0], &rxbuffer1[bytesRead]);
				CheckPayloads(buffer);
			}
			std::cout << messageToSend;
			if (sendto(serverSocket1, messageToSend.c_str(), messageToSend.length(), 0, (struct sockaddr*)&clientAddr1, clientAddrLen1) == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				LPSTR errorMessage = nullptr;
				FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMessage, 0, NULL);
				std::cerr << "Error sending data to client: " << errorMessage;
			}
		}

		{
			
			int clientAddrLen2 = sizeof(clientAddr2);

			memset(rxbuffer2, 0, sizeof(rxbuffer2));
			int bytesRead = recvfrom(serverSocket2, (char*)rxbuffer2, sizeof(rxbuffer2), 0, (struct sockaddr*)&clientAddr2, &clientAddrLen2);
			if (bytesRead == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
				{
					LPSTR errorMessage = nullptr;
					FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMessage, 0, NULL);
					std::cerr << "Error receiving data from UDP socket: " << errorMessage;
				}
			}
			else
			{
				std::cout << "Client: " << rxbuffer2;
				clientAddr1.sin_addr = clientAddr2.sin_addr;
			}

			std::string messageToSend = "PI\n";
			/*
			if (sendto(serverSocket2, messageToSend.c_str(), messageToSend.length(), 0, (struct sockaddr*)&clientAddr2, clientAddrLen2) == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				LPSTR errorMessage = nullptr;
				FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMessage, 0, NULL);
				std::cerr << "Error sending data to client: " << errorMessage;
			}
			*/
		}


		Update(hwnd, targetWnd);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	traccarThread.join();
	CleanupD2D();
	closesocket(serverSocket1);
	closesocket(serverSocket2);
	WSACleanup();
	return 0;
}
void ResizeRenderTarget(HWND hwnd)
{
	if (pRenderTarget)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
		);

		pRenderTarget->Resize(size);
	}
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_PAINT:
		RenderText(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		ResizeRenderTarget(hwnd);
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Update(HWND hwnd, HWND targetWnd)
{
	InvalidateRect(hwnd, 0, true);
	MSG msg = { };
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (!targetWnd)
	{
		std::cerr << "[Overlay] Target window not found!" << std::endl;
		targetWnd = FindWindow(NULL, TARGET_WINDOW_NAME);
		ShowWindow(hwnd, SW_HIDE);
	}

	UpdateOverlayPosition(hwnd, targetWnd);
}
void DrawOutlineText(const wchar_t* text, D2D1_RECT_F rect)
{
	D2D1_RECT_F outlineRect = rect;
	outlineRect.left -= 1;
	pRenderTarget->DrawText(text, wcslen(text), pTextFormat, outlineRect, pOutlineBrush);
	outlineRect.left += 1;
	pRenderTarget->DrawText(text, wcslen(text), pTextFormat, outlineRect, pOutlineBrush);
	outlineRect.left -= 1;
	outlineRect.top -= 1;
	pRenderTarget->DrawText(text, wcslen(text), pTextFormat, outlineRect, pOutlineBrush);
	outlineRect.top += 2;
	pRenderTarget->DrawText(text, wcslen(text), pTextFormat, outlineRect, pOutlineBrush);
	pRenderTarget->DrawText(text, wcslen(text), pTextFormat, rect, pBrush);
}
void RenderText(HWND hwnd) {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	if (!pRenderTarget)
	{
		HRESULT hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT,
				D2D1::PixelFormat(
					DXGI_FORMAT_B8G8R8A8_UNORM,
					D2D1_ALPHA_MODE_IGNORE)
			),
			D2D1::HwndRenderTargetProperties(
				hwnd,
				D2D1::SizeU(
					ps.rcPaint.right - ps.rcPaint.left,
					ps.rcPaint.bottom - ps.rcPaint.top)
			),
			&pRenderTarget);

		if (SUCCEEDED(hr))
		{
			pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White),
				&pBrush);

			pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(0x010101),
				&pOutlineBrush);
		}
	}

	pRenderTarget->BeginDraw();
	pRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)); // Fully transparent background

	D2D1_RECT_F layoutRect = D2D1::RectF(
		static_cast<FLOAT>(ps.rcPaint.left),
		static_cast<FLOAT>(ps.rcPaint.top),
		static_cast<FLOAT>(ps.rcPaint.right),
		static_cast<FLOAT>(ps.rcPaint.bottom)
	);


	std::regex pattern("Temp: (\\d+) C, R: (\\d+) KB/s, T: (\\d+) KB/s, RSSI: (-?\\d+), SNR: (-?\\d+)");
	std::smatch matches;
	std::string telemetry(rxbuffer2);
	std::lock_guard<std::mutex> lock(sharedMutex);
	if (std::regex_search(telemetry, matches, pattern))
	{
		tel.pi_temp = std::stoi(matches[1]);
		tel.pi_read_speed = std::stoi(matches[2]);
		tel.pi_write_speed = std::stoi(matches[3]);
		tel.pi_rssi = std::stoi(matches[4]);
		tel.pi_snr = std::stoi(matches[5]);

		//swprintf(telemetryBuffer, 128, L"CPU %4d °C\n↓ %4d KB/s\n↑ %4d KB/s\nRSSI: %4d\nSNR: %5d", temp, read_speed, write_speed, rssi, snr);
	}
	static wchar_t leftBuffer[512], centerBuffer[512], rightBuffer[512];
	time_t currentTime;
	time(&currentTime);
	std::wstring bottomText;
	if (currentTime % 2)
	{
		if (((float)tel.voltage) / (serCells * 10) < 2.6f)
			bottomText = L"LAND NOW!!!";
		else if (((float)tel.voltage) / (serCells * 10) < 3.0f)
			bottomText = L"BATTERY LOW!";
	}
	int distanceToHome = calculateHaversine((double)tel.latitude / 10000000.0, (double)tel.longitude / 10000000.0, homeLat, homeLon);
	swprintf(leftBuffer, 512, L"🌡️%4d °C\n↓%4d KB/s\n↑%4d KB/s\n\n\n%4.1fV 🔋 %4.2fV\n%4.1fA  %4dmAh\n\nRSSI %4d\nSNR %5d",
		tel.pi_temp, tel.pi_read_speed, tel.pi_write_speed, ((float)tel.voltage) / 10, ((float)tel.voltage) / (10 * serCells), ((float)tel.current) / 10, tel.capacity, tel.pi_rssi, tel.pi_snr);
	swprintf(centerBuffer, 512, L"%s\n%s\n|\n\n\n\n%s\n\n\n\n\n\n\n\n\n%s", convert_to_wstring(tel.flightMode).c_str(), Compass(tel.heading / 100).c_str(), bottomText.c_str(), controller.GetFlags().c_str());
	swprintf(rightBuffer, 512, L"%s\n\nLat%10.6f\nLon%10.6f\n↕️%4dm 🧭%3d°\n🛰️%4d­­\n⏱%4d km/h\n%3.1f km/h/A\n🏠%5dm\n\n\nP %5.1f°\nR %5.1f°\nY %5.1f°", convert_to_wstring(ctime(&currentTime)).c_str(), ((float)tel.latitude) / 10000000.f, ((float)tel.longitude) / 10000000.f, tel.altitude, tel.heading / 100, tel.satellites, tel.groundspeed / 10, (tel.groundspeed / 10) / (((float)tel.current) / 10), distanceToHome, RADTODEG(tel.pitch) / 10000.f, RADTODEG(tel.roll) / 10000.f, RADTODEG(tel.yaw) / 10000.f);



	pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	DrawOutlineText(leftBuffer, layoutRect);

	pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	DrawOutlineText(centerBuffer, layoutRect);

	pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	DrawOutlineText(rightBuffer, layoutRect);

	pRenderTarget->EndDraw();

	EndPaint(hwnd, &ps);
}

void InitD2D(HWND hwnd)
{
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
	pDWriteFactory->CreateTextFormat(
		L"Consolas",
		NULL,
		DWRITE_FONT_WEIGHT_REGULAR,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		30.0f,
		L"",
		&pTextFormat);
}

void CleanupD2D()
{
	if (pBrush) pBrush->Release();
	if (pTextFormat) pTextFormat->Release();
	if (pDWriteFactory) pDWriteFactory->Release();
	if (pRenderTarget) pRenderTarget->Release();
	if (pFactory) pFactory->Release();
}

void UpdateOverlayPosition(HWND hwnd, HWND targetWnd) {
	RECT rect;
	if (GetWindowRect(targetWnd, &rect)) {
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
}
int Http_Post(const char* domain, int port, const char* path, char* body, uint16_t bodyLen, char* retBuffer, int bufferLen) {
	bool flag = false;
	uint16_t recvLen = 0;

	// Initialize Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed\n");
		return -1;
	}

	// Allocate memory for request
	static char request[512];
	snprintf(request, 512, "POST %s HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: Keep-Alive\r\nHost: %s\r\nContent-Length: %d\r\n\r\n",
		path, domain, bodyLen);

	// Create socket
	SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == INVALID_SOCKET) {
		fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	// Resolve domain to IP address
	struct addrinfo hints, * res, * p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // AF_INET for IPv4
	hints.ai_socktype = SOCK_STREAM;

	int status;
	if ((status = getaddrinfo(domain, NULL, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo fail: %s\n", gai_strerror(status));
		closesocket(fd);
		WSACleanup();
		return -1;
	}

	// Setup sockaddr_in
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr;

	if (connect(fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
		fprintf(stderr, "socket connect fail: %d\n", WSAGetLastError());
		closesocket(fd);
		WSACleanup();
		freeaddrinfo(res);
		return -1;
	}

	freeaddrinfo(res);

	// Send request headers
	if (send(fd, request, (int)strlen(request), 0) == SOCKET_ERROR) {
		fprintf(stderr, "socket send fail: %d\n", WSAGetLastError());
		closesocket(fd);
		WSACleanup();
		return -1;
	}

	// Send request body
	if (send(fd, body, bodyLen, 0) == SOCKET_ERROR) {
		fprintf(stderr, "socket send fail: %d\n", WSAGetLastError());
		closesocket(fd);
		WSACleanup();
		return -1;
	}

	fd_set fds;
	struct timeval timeout = { 12, 0 };
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	while (!flag) {
		int ret = select(0, &fds, NULL, NULL, &timeout);
		switch (ret) {
		case SOCKET_ERROR:
			fprintf(stderr, "select error: %d\n", WSAGetLastError());
			flag = true;
			break;
		case 0:
			fprintf(stderr, "select timeout\n");
			flag = true;
			break;
		default:
			if (FD_ISSET(fd, &fds)) {
				memset(retBuffer, 0, bufferLen);
				ret = recv(fd, retBuffer, bufferLen, 0);
				recvLen += ret;
				if (ret == SOCKET_ERROR) {
					fprintf(stderr, "recv error: %d\n", WSAGetLastError());
					flag = true;
					break;
				}
				else if (ret == 0) {
					break;
				}
				else if (ret < bufferLen) {
					closesocket(fd);
					WSACleanup();
					return recvLen;
				}
			}
			break;
		}
	}

	closesocket(fd);
	WSACleanup();
	return -1;
}
void TraccarUpdate()
{
	char requestPath[256];
	char buffer[1024];
	while (true)
	{
		{
			std::lock_guard<std::mutex> lock(sharedMutex);
			snprintf(requestPath, sizeof(requestPath), "/?id=123456789&timestamp=%d&lat=%f&lon=%f&speed=%d&altitude=%d", (int)time(NULL), ((double)tel.latitude) / 10000000.0, ((double)tel.longitude) / 10000000.0, tel.groundspeed / 10, tel.altitude);
		}
		if (Http_Post("tutos.ddns.net", 8082, requestPath, NULL, 0, buffer, sizeof(buffer)) < 0)
			fprintf(stderr, "send location to traccar fail\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

int initializeSocket(int port, struct sockaddr_in& serverAddr) {
	int serverSocket;

	// Create a socket
	if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
		std::cerr << "Socket creation error (port " << port << "): " << WSAGetLastError() << std::endl;
		return INVALID_SOCKET;
	}

	// Set up the sockaddr_in structure
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	const int enable = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) failed (port " << port << "): " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		return INVALID_SOCKET;
	}

	if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Binding failed (port " << port << "): " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		return INVALID_SOCKET;
	}

	u_long mode = 1; // Non-blocking mode
	if (ioctlsocket(serverSocket, FIONBIO, &mode) != NO_ERROR) {
		std::cerr << "ioctlsocket failed (port " << port << "): " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		return INVALID_SOCKET;
	}

	std::cout << "Server listening on port " << port << std::endl;
	return serverSocket; // Return the valid socket descriptor
}