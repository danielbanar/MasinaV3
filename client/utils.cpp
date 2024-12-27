#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <regex>
struct NetworkUsage
{
    unsigned long rx_bytes;
    unsigned long tx_bytes;
};
NetworkUsage getNetworkUsage()
{
    std::ifstream netDevFile("/proc/net/dev");
    std::string line;
    NetworkUsage usage = {0, 0};

    // Skip the first two lines (header lines)
    std::getline(netDevFile, line);
    std::getline(netDevFile, line);

    while (std::getline(netDevFile, line))
    {
        std::istringstream iss(line);
        std::string iface;
        iss >> iface;

        // Remove trailing colon from the interface name
        iface.pop_back();

        // Skip the unwanted interfaces
        if (iface != "lo")
        {
            unsigned long rx_bytes, tx_bytes;
            iss >> rx_bytes; // read rx_bytes
            for (int i = 0; i < 7; ++i)
                iss >> tx_bytes; // skip to tx_bytes
            iss >> tx_bytes;     // read tx_bytes

            usage.rx_bytes += rx_bytes;
            usage.tx_bytes += tx_bytes;
        }
    }

    return usage;
}

// Function to read CPU temperature from sysfs
int get_cpu_temperature()
{
    std::ifstream file("/sys/devices/virtual/mstar/msys/TEMP_R");
    if (!file.is_open())
    {
        std::cerr << "Error: Unable to open temperature file." << std::endl;
        return -1;
    }

    std::string line;
    std::getline(file, line);
    file.close();

    // Parse the temperature value from the line
    size_t pos = line.find("Temperature ");
    if (pos == std::string::npos)
    {
        std::cerr << "Error: Unexpected file format." << std::endl;
        return -1;
    }

    try
    {
        int temperature = std::stoi(line.substr(pos + 12)); // Extract temperature after "Temperature "
        return temperature;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: Failed to parse temperature: " << e.what() << std::endl;
        return -1;
    }
}

void getSignalStrength(int& rssi, int& snr) {
    const char* command = "qmicli --device=/dev/cdc-wdm0 --nas-get-signal-strength";

    // Open a pipe to execute the command
    FILE* fp = popen(command, "r");
    if (fp == nullptr) {
        std::cerr << "Failed to run command" << std::endl;
        rssi = -1;  // Return -1 if command fails
        snr = -1;   // Return -1 if command fails
        return;
    }

    // Read the output of the command line by line
    char buffer[256];
    std::string output;
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        output += buffer;
    }
	std::cout<< buffer;
    // Close the pipe
    fclose(fp);

    // Log the raw output for debugging
    std::cout << "Command Output:\n" << output << std::endl;

    // Use regex to extract RSSI and SNR values
    std::regex rssiRegex("RSSI:\\s*.*?([-\\d]+) dBm");
    std::regex snrRegex("SNR:\\s*.*?([\\d\\.]+) dB");
    std::smatch match;

    // Parse RSSI
    if (std::regex_search(output, match, rssiRegex) && match.size() > 1) {
        rssi = std::stoi(match[1].str());
    } else {
        std::cerr << "Failed to parse RSSI" << std::endl;
        rssi = -1;
    }

    // Parse SNR
    if (std::regex_search(output, match, snrRegex) && match.size() > 1) {
        snr = static_cast<int>(std::stof(match[1].str()));
    } else {
        std::cerr << "Failed to parse SNR" << std::endl;
        snr = -1;
    }
}
