#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <netdb.h>
#include <regex>
#include <fstream>
#include <fcntl.h>
#include <ctime>
#include <cstdlib>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "/usr/include/asm-generic/termbits.h"
#include "/usr/include/asm-generic/ioctls.h"
#include <errno.h>
#include <cmath>
#define CRSF_CHANNEL_VALUE_MIN  172
#define CRSF_CHANNEL_VALUE_1000 191
#define CRSF_CHANNEL_VALUE_MID  992
#define CRSF_CHANNEL_VALUE_2000 1792
#define CRSF_CHANNEL_VALUE_MAX  1811
#define BUFFER_SIZE 128
#define RADTODEG(radians) ((radians) * (180.0 / M_PI))

int LOCAL_TIMEOUT = 300000;
int FAILSAFE_TIMEOUT = 5000;
int STABILIZE_TIMEOUT = 250;
std::string hostname;

int get_cpu_temperature();
struct NetworkUsage
{
	unsigned long rx_bytes;
	unsigned long tx_bytes;
};
NetworkUsage getNetworkUsage();
void getSignalStrength(int& rssi, int& snr);
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
uint8_t CRC(const uint8_t* data, size_t start, size_t length)
{
	uint8_t crc = 0;
	size_t end = start + length;

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

bool readConfig(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open configuration file " << filename << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string key, value;
		if (std::getline(iss, key, '=') && std::getline(iss, value)) {
			if (key == "host")
				hostname = value;
			else if (key == "LOCAL_TIMEOUT")
				LOCAL_TIMEOUT = std::stoi(value);
			else if (key == "FAILSAFE_TIMEOUT")
				FAILSAFE_TIMEOUT = std::stoi(value);
			else if (key == "STABILIZE_TIMEOUT")
				STABILIZE_TIMEOUT = std::stoi(value);
		}
	}
	return true;
}

int initializeSocket(const std::string& address, int port, struct sockaddr_in& serverAddr) {
	int sockfd;
	struct addrinfo hints, * res;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&serverAddr, 0, sizeof(serverAddr));
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_DGRAM;  // UDP

	// Check if the input is an IP address
	if (inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr) == 1) {
		// It's an IP address
		serverAddr.sin_family = AF_INET;
	}
	else {
		// It's a hostname, resolve it
		if (getaddrinfo(address.c_str(), NULL, &hints, &res) != 0) {
			perror("getaddrinfo failed");
			exit(EXIT_FAILURE);
		}

		// Copy the resolved address to serverAddr
		serverAddr = *(struct sockaddr_in*)(res->ai_addr);
		freeaddrinfo(res);
	}

	serverAddr.sin_port = htons(port);

	// Set socket to non-blocking
	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	return sockfd;
}

void receiveMessages(int sockfd, struct sockaddr_in& serverAddr)
{
	char buffer[BUFFER_SIZE];
	socklen_t addrLen = sizeof(serverAddr);

	while (true)
	{
		usleep(1000);
		int len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&serverAddr, &addrLen);
		if (len == -1)
		{

		}
		if (len > 0)
		{
			buffer[len] = '\0';
			std::cout << "Received: " << buffer;
		}
	}
}
void PiTelemetry()
{
	struct sockaddr_in serverAddr;
	int sockfd = initializeSocket(hostname, 2224, serverAddr);

	std::thread receiveThread(receiveMessages, sockfd, std::ref(serverAddr));

	while (true)
	{
		const double interval = 0.5; // interval in seconds
		const double bytes_to_kb = 1024.0;
		NetworkUsage usage1 = getNetworkUsage();
		std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(interval * 1000)));
		NetworkUsage usage2 = getNetworkUsage();
		unsigned long rx_diff = usage2.rx_bytes - usage1.rx_bytes;
		unsigned long tx_diff = usage2.tx_bytes - usage1.tx_bytes;
		unsigned long rx_kbps = static_cast<unsigned long>(rx_diff / bytes_to_kb / interval);
		unsigned long tx_kbps = static_cast<unsigned long>(tx_diff / bytes_to_kb / interval);
		int rssi = 0;
		int snr = 0;
		getSignalStrength(rssi, snr);
		std::string telemetryString = "Temp: " + std::to_string(get_cpu_temperature()) + " C, R: " + std::to_string(rx_kbps) + " KB/s, T: " + std::to_string(tx_kbps) + " KB/s, RSSI: " + std::to_string(rssi) + ", SNR: " + std::to_string(snr) + "\n\0";

		sendto(sockfd, telemetryString.c_str(), telemetryString.length(), 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr));
	}

	receiveThread.join();
	close(sockfd);
}
int main()
{
	if (!readConfig("/root/config.txt")) {
		return 1;
	}

	std::cout << "Using hostname: " << hostname << std::endl;
	std::cout << "LOCAL_TIMEOUT: " << LOCAL_TIMEOUT << std::endl;
	std::cout << "FAILSAFE_TIMEOUT: " << FAILSAFE_TIMEOUT << std::endl;
	std::cout << "STABILIZE_TIMEOUT: " << STABILIZE_TIMEOUT << std::endl;

	std::thread PiTelemetryThread(PiTelemetry);

	int serialPort = open("/dev/ttyS2", O_RDWR);
	int baudrate = 420000;
	struct termios2 tio;
	ioctl(serialPort, TCGETS2, &tio);
	tio.c_cflag &= ~CBAUD;
	tio.c_cflag |= BOTHER;
	tio.c_ispeed = baudrate;
	tio.c_ospeed = baudrate;
	tio.c_cc[VTIME] = 10;
	tio.c_cc[VMIN] = 64;

	tio.c_cflag = 7344;
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_lflag = 0;


	if (ioctl(serialPort, TCSETS2, &tio) != 0)
		printf("serial error");

	// Set the serial port to non-blocking mode
	int flags = fcntl(serialPort, F_GETFL, 0);
	if (flags == -1) {
		perror("Failed to get file status flags");
		close(serialPort);
		return 1;
	}

	flags |= O_NONBLOCK;
	if (fcntl(serialPort, F_SETFL, flags) == -1) {
		perror("Failed to set file status flags");
		close(serialPort);
		return 1;
	}

	struct sockaddr_in serverAddr;
	int sockfd = initializeSocket(hostname, 2223, serverAddr);
	uint8_t dummybuf[5] = "INIT";
	sendto(sockfd, dummybuf, 5, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	std::cout << "INIT SENT";
	while (true)
	{
		usleep(1000);
		static uint16_t channels[16] = { 992, 992, 1716, 992, 191, 191, 191, 191, 997, 997, 997, 997,   0,   0,1811, 1811 };
		static bool fsMode = false;
		static auto lastValidPayload = std::chrono::high_resolution_clock::now();
		static auto lastSentPayload = std::chrono::high_resolution_clock::now();
		uint8_t serialBuffer[128] = { 0 };
		int serialReadBytes = read(serialPort, &serialBuffer, sizeof(serialBuffer));
		try
		{
			if (serialReadBytes < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
				}
				else
				{
					perror("Failed to read from serial port");
					usleep(100000);
				}
			}
			else if (serialReadBytes == 0)
			{
				printf("EOF\n");
			}
			else
			{
				sendto(sockfd, serialBuffer, serialReadBytes, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
			}

			char rxBuffer[128];
			sockaddr_in clientAddr{};
			socklen_t addrLen = sizeof(clientAddr);
			ssize_t bytesRead = recvfrom(sockfd, rxBuffer, sizeof(rxBuffer), 0, (struct sockaddr*)&clientAddr, &addrLen);
			if (bytesRead == -1)
			{
				if (errno == EWOULDBLOCK || errno == EAGAIN)
				{
					auto currentTime = std::chrono::high_resolution_clock::now();
					auto elapsedTimeValid = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastValidPayload).count();
					if (elapsedTimeValid >= LOCAL_TIMEOUT)
					{
						//No data for 5m - Switch to local controller
						std::cerr << "LOCAL_TIMEOUT\n";
						std::system("gpio clear 10");
					}
					else if (elapsedTimeValid >= FAILSAFE_TIMEOUT)
					{
						//No data for 5s - Failsafe
						//std::cerr << "FAILSAFE_TIMEOUT\n";
						channels[6] = fsMode ? CRSF_CHANNEL_VALUE_2000 : CRSF_CHANNEL_VALUE_MID;
					}
					else if (elapsedTimeValid >= STABILIZE_TIMEOUT)
					{
						//No data for 250ms - STABILIZE
						//std::cerr << "STABILIZE_TIMEOUT\n";
						channels[0] = CRSF_CHANNEL_VALUE_MID;//ROLL
						channels[1] = CRSF_CHANNEL_VALUE_MID;//PITCH
						channels[2] = CRSF_CHANNEL_VALUE_MID;//YAW
						channels[3] = CRSF_CHANNEL_VALUE_MID;//THROTTLE
						channels[5] = CRSF_CHANNEL_VALUE_MIN;//ANGLE MODE MODE

					}
					auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastSentPayload).count();
					if (elapsedTime > 20)
					{
						static uint8_t payload[26];// = "\xC8\x18\x16\xE0\x03\x1F\xAD\xC1\xF7\x8B\x5F\xFC\xE2\x17\xE5\x2B\x5F\xF9\xCA\x07\x00\x00\x4C\x7C\xE2\x17";
						payload[0] = 0xC8;
						payload[1] = 0x18;
						payload[2] = 0x16;
						payload[3] = (channels[0] & 0xFF);
						payload[4] = ((channels[0] >> 8) & 0x07) | ((channels[1] & 0x1F) << 3);
						payload[5] = ((channels[1] >> 5) & 0x3F) | ((channels[2] & 0x03) << 6);
						payload[6] = ((channels[2] >> 2) & 0xFF);
						payload[7] = ((channels[2] >> 10) & 0x01) | ((channels[3] & 0x7F) << 1);
						payload[8] = ((channels[3] >> 7) & 0x0F) | ((channels[4] & 0x0F) << 4);
						payload[9] = ((channels[4] >> 4) & 0x7F) | ((channels[5] & 0x01) << 7);
						payload[10] = ((channels[5] >> 1) & 0xFF);
						payload[11] = ((channels[5] >> 9) & 0x03) | ((channels[6] & 0x3F) << 2);
						payload[12] = ((channels[6] >> 6) & 0x1F) | ((channels[7] & 0x07) << 5);
						payload[13] = ((channels[7] >> 3) & 0xFF);
						payload[14] = (channels[8] & 0xFF);
						payload[15] = ((channels[8] >> 8) & 0x07) | ((channels[9] & 0x1F) << 3);
						payload[16] = ((channels[9] >> 5) & 0x3F) | ((channels[10] & 0x03) << 6);
						payload[17] = ((channels[10] >> 2) & 0xFF);
						payload[18] = ((channels[10] >> 10) & 0x01) | ((channels[11] & 0x7F) << 1);
						payload[19] = ((channels[11] >> 7) & 0x0F) | ((channels[12] & 0x0F) << 4);
						payload[20] = ((channels[12] >> 4) & 0x7F) | ((channels[13] & 0x01) << 7);
						payload[21] = ((channels[13] >> 1) & 0xFF);
						payload[22] = ((channels[13] >> 9) & 0x03) | ((channels[14] & 0x3F) << 2);
						payload[23] = ((channels[14] >> 6) & 0x1F) | ((channels[15] & 0x07) << 5);
						payload[24] = ((channels[15] >> 3) & 0xFF);
						payload[25] = CRC(payload, 2, 0x18 - 1);
						ssize_t bytes_written = write(serialPort, payload, 26);
						/*static uint8_t linkPayload[15] = "\xC8\x0C\x14\x10\x17\x64\x05\x00\x01\x01\x00\x00\x00\x59"; //Dummy data
						bytes_written = write(serialPort, linkPayload, 14);*/
						lastSentPayload = currentTime;
					}
					usleep(5000);
				}
				else
				{
					perror("Error");
					usleep(100000);
				}
			}
			else if (bytesRead == 0)
			{
				std::cout << "Connection closed by the server" << std::endl;
				usleep(100000);
			}
			else if (bytesRead > 0)
			{
				rxBuffer[bytesRead] = '\0';
				static std::regex regexPattern("N(-?\\d+)RX(-?\\d+)RY(-?\\d+)LX(-?\\d+)LY(-?\\d+)SA(-?\\d+)SB(-?\\d+)SC(-?\\d+)REM(-?\\d+)FSM(-?\\d+)CRC(-?\\d+)\\n");
				std::cmatch matches;
				if (std::regex_search(rxBuffer, matches, regexPattern))
				{
					static uint16_t controls[10];
					static unsigned long lastN = 0;
					for (int i = 0; i < 10; i++)
						controls[i] = std::stol(matches[i + 1]) & 0xFFFF;
					uint16_t crc_recv = std::stoi(matches[11]);
					uint16_t crc_calc = CRC16(controls, 10);
					//printf("CRC: recv=%d calc=%d\n", crc_recv, crc_calc);
					if (crc_recv != crc_calc)
					{
						continue;
					}
					//Valid payload
					unsigned long N = std::stol(matches[1]);
					if (lastN < N)	//In order
					{
						lastN = N;
						lastValidPayload = std::chrono::high_resolution_clock::now();
						memcpy(channels, controls + 1, 8 * sizeof(uint16_t));
						bool remote = std::stoi(matches[9]) & 1;
						fsMode = std::stoi(matches[10]) & 1;
						static bool lastRemoteState = false;
						if (remote != lastRemoteState) {
							remote ? std::system("gpio set 10") : std::system("gpio clear 10");
							lastRemoteState = remote;
						}
						
						/*for(int i = 0;i<10;i++)
							printf("%d ",controls[i]);
						printf("\n");*/
					}
					else
					{
						auto currentTime = std::chrono::high_resolution_clock::now();
						auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastValidPayload).count();
						if (elapsedTime >= 2)
							lastN = 0;
					}
				}
				else
				{
					std::cout << "Received: " << rxBuffer;
				}

			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
			usleep(10000); // Sleep for 1 second on exception
		}
	}
	close(sockfd);
	close(serialPort);
	PiTelemetryThread.join();
	return 0;
}
