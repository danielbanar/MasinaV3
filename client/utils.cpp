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
struct ModemStats
{
    int16_t rssi, rsrq, rsrp;
    int8_t snr;
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
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open())
    {
        std::cerr << "Error: Unable to open temperature file." << std::endl;
        return -1;
    }

    std::string temp_str;
    std::getline(file, temp_str);
    file.close();

    float temp = std::stof(temp_str) / 1000.0; // Convert to Celsius
    return temp;
}

// Function to extract modem index from mmcli -L output
int getModemIndex()
{
    std::string command_output;
    std::string mmcli_command = "mmcli -L";

    // Run mmcli -L and read the output
    FILE *pipe = popen(mmcli_command.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "Error executing command: " << mmcli_command << std::endl;
        return -1;
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        command_output += buffer;
    }

    pclose(pipe);

    // Parse the output to find the modem index
    std::istringstream iss(command_output);
    std::string line;
    while (std::getline(iss, line))
    {
        std::cout << line << std::endl;
        size_t found = line.find("/org/freedesktop/ModemManager1/Modem/");
        if (found != std::string::npos)
        {
            size_t index_start = found + sizeof("/org/freedesktop/ModemManager1/Modem/") - 1;
            size_t index_end = line.find(" -", index_start);
            std::string index_str = line.substr(index_start, index_end - index_start);
            return std::stoi(index_str);
        }
    }

    return -1; // Modem index not found
}

ModemStats getModemStats(int modemIndex)
{
    char command[64];
    sprintf(command, "mmcli -m %d --signal-get", modemIndex);
    FILE *pipe = popen(command, "r");
    if (!pipe)
    {
        std::cerr << "Error executing command: mmcli --signal-get" << std::endl;
        return {0};
    }
    char mmcli_output[256];
    std::string result = "";
    while (fgets(mmcli_output, sizeof(mmcli_output), pipe) != nullptr)
    {
        result += mmcli_output;
    }
    pclose(pipe);

    ModemStats ms;

    std::regex rssiRegex("rssi: ([\\d.-]+) dBm");
    std::regex rsrqRegex("rsrq: ([\\d.-]+) dB");
    std::regex rsrpRegex("rsrp: ([\\d.-]+) dBm");
    std::regex snRegex("s/n: ([\\d.-]+) dB");

    // Match objects
    std::smatch rssiMatch, rsrqMatch, rsrpMatch, snMatch;

    if (std::regex_search(result, rssiMatch, rssiRegex))
        ms.rssi = std::stoi(rssiMatch[1]);

    if (std::regex_search(result, rsrqMatch, rsrqRegex))
        ms.rsrq = std::stoi(rsrqMatch[1]);

    if (std::regex_search(result, rsrpMatch, rsrpRegex))
        ms.rsrp = std::stoi(rsrpMatch[1]);

    if (std::regex_search(result, snMatch, snRegex))
        ms.snr = std::stoi(snMatch[1]);
    return ms;
}
