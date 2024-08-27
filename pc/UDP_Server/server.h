#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <iostream>
#include <ctime>
#include <locale>
#include <string>
#pragma comment (lib,"Gdiplus.lib")
#define M_PI 3.14159265358979323846
#define RADTODEG(radians) ((radians) * (180.0 / M_PI))
#define DEGTORAD(degrees) ((degrees) * (M_PI / 180.0))

using namespace Gdiplus;
double calculateAngle(double lat1, double lon1, double lat2, double lon2);
double calculateHaversine(double lat1, double lon1, double lat2, double lon2);
uint8_t CRC(const std::vector<uint8_t>& data, std::size_t start, std::size_t length);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void RenderText(HWND hwnd);
void Update(HWND hwnd, HWND targetWnd);
void UpdateOverlayPosition(HWND hwnd, HWND targetWnd);
void TraccarUpdate();
void printVectorHex(const std::vector<uint8_t>& vec, std::size_t maxLength);