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
#include <time.h>
#include <mutex>
#include <cmath>
#define M_PI 3.14159265358979323846
#define RADTODEG(radians) ((radians) * (180.0 / M_PI))
#define DEGTORAD(degrees) ((degrees) * (M_PI / 180.0))
double calculateAngle(double lat1, double lon1, double lat2, double lon2)
{
    double dLon = DEGTORAD(lon2 - lon1);
    double y = sin(dLon) * cos(DEGTORAD(lat2));
    double x = cos(DEGTORAD(lat1)) * sin(DEGTORAD(lat2)) -
        sin(DEGTORAD(lat1)) * cos(DEGTORAD(lat2)) * cos(dLon);
    double angle = atan2(y, x);
    angle = RADTODEG(angle);
    if (angle < 0)
    {
        angle += 360.0;
    }
    return angle;
}
double calculateHaversine(double lat1, double lon1, double lat2, double lon2)
{
    // Convert latitude and longitude from degrees to radians
    lat1 = DEGTORAD(lat1);
    lon1 = DEGTORAD(lon1);
    lat2 = DEGTORAD(lat2);
    lon2 = DEGTORAD(lon2);

    // Calculate differences
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    // Haversine formula
    double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    const double EarthRadiusKm = 6371.0; // Earth radius in kilometers
    // Distance in kilometers
    double distance = EarthRadiusKm * c;

    return distance * 1000.0;
}
uint8_t CRC(const std::vector<uint8_t>& data, std::size_t start, std::size_t length)
{
	uint8_t crc = 0;
	std::size_t end = start + length;

	if (end > data.size())
		end = data.size();

	for (std::size_t i = start; i < end; ++i)
	{
		crc ^= data[i];

		for (uint8_t j = 0; j < 8; ++j)
		{
			if (crc & 0x80)
				crc = (crc << 1) ^ 0xD5;
			else
				crc <<= 1;
		}
	}

	return crc;
}
void printHex(const char* arr, size_t length) 
{
	for (size_t i = 0; i < length; i++) {
		printf("%02X ", (unsigned char)arr[i]);
	}
	printf("\n");
}
void printVectorHex(const std::vector<uint8_t>& vec, std::size_t maxLength) 
{
	std::size_t count = 0;
	for (const auto& val : vec) {
		if (count >= maxLength) break;
		printf("%02X ", val);
		count++;
	}
	printf("\n");
}
