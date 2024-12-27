#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

void handle_resolution_request(const std::string& request) {
    if (request.find("setResolution=") != std::string::npos) {
        size_t pos = request.find("setResolution=");
        if (pos != std::string::npos) {
            std::string resolution = request.substr(pos + 14);  // Skip over "setResolution="
            resolution = resolution.substr(0, resolution.find(' '));  // Get resolution value

            std::cout << "Received resolution command with value: " << resolution << std::endl;

            // Construct the shell command for resolution
            std::string command = "yaml-cli -s .video0.size " + resolution;
            std::cout << "Executing command: " << command << std::endl;

            // Execute the shell command
            std::system(command.c_str());
        }
    }
}

void handle_combined_fps_request(const std::string& request) {
    if (request.find("setFps=") != std::string::npos) {
        size_t pos = request.find("setFps=");
        if (pos != std::string::npos) {
            std::string fps_value = request.substr(pos + 7);  // Skip over "setFps="
            fps_value = fps_value.substr(0, fps_value.find(' '));  // Get FPS value

            std::cout << "Received combined FPS command with value: " << fps_value << std::endl;

            // Construct the shell command for Sensor FPS
            std::string sensor_fps_command = "echo setfps 0 " + fps_value + " > /proc/mi_modules/mi_sensor/mi_sensor0";
            std::cout << "Executing sensor FPS command: " << sensor_fps_command << std::endl;
            std::system(sensor_fps_command.c_str());

            // Construct the shell command for Video FPS (using yaml-cli)
            std::string video_fps_command = "yaml-cli -s .video0.fps " + fps_value;
            std::cout << "Executing video FPS command: " << video_fps_command << std::endl;
            std::system(video_fps_command.c_str());
        }
    }
}

void handle_bitrate_request(const std::string& request) {
    if (request.find("setBitrate=") != std::string::npos) {
        size_t pos = request.find("setBitrate=");
        if (pos != std::string::npos) {
            std::string bitrate_value = request.substr(pos + 11);  // Skip over "setBitrate="
            bitrate_value = bitrate_value.substr(0, bitrate_value.find(' '));  // Get bitrate value

            std::cout << "Received bitrate command with value: " << bitrate_value << std::endl;

           // Construct the URL for setting the bitrate
            std::string url = "http://localhost/api/v1/set?video0.bitrate=" + bitrate_value;
            std::cout << "Sending request to URL: " << url << std::endl;

            // You can make an actual HTTP request here using a library like cURL (not shown in this example)
            // For now, we're just printing the URL.
            std::system(("curl " + url).c_str());

            // Construct the shell command for Video Bitrate (using yaml-cli)
            std::string video_bitrate_command = "yaml-cli -s .video0.bitrate " + bitrate_value;
            std::cout << "Executing video bitrate command: " << video_bitrate_command << std::endl;
            std::system(video_bitrate_command.c_str());
        }
    }
}

void handle_ir_request(const std::string& request) {
    if (request.find("setIR=") != std::string::npos) {
        size_t pos = request.find("setIR=");
        if (pos != std::string::npos) {
            std::string ir_state = request.substr(pos + 6);  // Skip over "setIR="
            ir_state = ir_state.substr(0, ir_state.find(' '));  // Get IR state (ON or OFF)

            if (ir_state == "ON") {
                // Execute IR ON commands
                std::cout << "IR ON" << std::endl;
                std::system("gpio clear 24");
                std::system("gpio set 23");
            } else if (ir_state == "OFF") {
                // Execute IR OFF commands
                std::cout << "IR OFF" << std::endl;
                std::system("gpio clear 23");
                std::system("gpio set 24");
            }
        }
    }
}

void handle_restart_request(const std::string& request) {
    if (request.find("restart=") != std::string::npos) {
        std::cout << "Restarting..." << std::endl;
        std::system("killall -1 majestic");
    }
}

void send_html_response(int client_fd) {
    std::string html_content = R"(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Masina web control server</title>
        <style>
            body { font-family: Arial, sans-serif; text-align: center; margin-top: 10px; }
            #combinedFpsValue, #bitrateValue { font-size: 20px; margin-top: 10px; }
            select { width: 200px; font-size: 16px; }
            button { font-size: 16px; padding: 15px; margin-top: 10px; }
        </style>
    </head>
    <body>
        <h1>Masina web control server</h1>

        <!-- Resolution Dropdown -->
        <h2>Resolution</h2>
        <select id="resolutionDropdown">
            <option value="2560x1920">2560x1920</option>
            <option value="2560x1440">2560x1440</option>
            <option value="1920x1080">1920x1080</option>
            <option value="1080x720">1080x720</option>
            <option value="960x720">960x720</option>
            <option value="704x576">704x576</option>
            <option value="640x480">640x480</option>
        </select>
        
        <!-- Combined FPS Slider -->
        <h2>FPS Control</h2>
        <input type="range" id="combinedFpsSlider" min="10" max="120" value="30" step="10" style="width: 80%;" />
        <div id="combinedFpsValue">Sensor FPS: 30, Video FPS: 30</div>

        <!-- Bitrate Slider -->
        <h2>Bitrate</h2>
        <input type="range" id="bitrateSlider" min="0" max="8192" value="1024" step="256" style="width: 80%;" />
        <div id="bitrateValue">Bitrate: 1024</div>

        <!-- IR Control -->
        <h2>IR Control</h2>
        <button id="irOnButton">IR ON</button>
        <button id="irOffButton">IR OFF</button>

        <!-- Restart Button -->
        <h2>Restart</h2>
        <button id="restartButton">Restart</button>

        <script>
            const resolutionDropdown = document.getElementById("resolutionDropdown");
            const combinedFpsSlider = document.getElementById("combinedFpsSlider");
            const combinedFpsValue = document.getElementById("combinedFpsValue");
            const bitrateSlider = document.getElementById("bitrateSlider");
            const bitrateValue = document.getElementById("bitrateValue");
            const irOnButton = document.getElementById("irOnButton");
            const irOffButton = document.getElementById("irOffButton");
            const restartButton = document.getElementById("restartButton");

            // Handle resolution change
            resolutionDropdown.onchange = function() {
                const resolution = this.value;
                fetch('/setResolution=' + resolution)
                    .then(response => response.text())
                    .then(data => console.log('Resolution Success:', data))
                    .catch((error) => console.error('Resolution Error:', error));
            };

            // Handle combined FPS slider change (for both Sensor and Video FPS)
            combinedFpsSlider.oninput = function() {
                const fps = this.value;
                combinedFpsValue.innerHTML = "Sensor FPS: " + fps + ", Video FPS: " + fps;
                
                // Update Sensor FPS via /proc/mi_modules/mi_sensor/mi_sensor0
                fetch('/setFps=' + fps)
                    .then(response => response.text())
                    .then(data => console.log('Sensor FPS Success:', data))
                    .catch((error) => console.error('Sensor FPS Error:', error));

                // Update Video FPS via yaml-cli
                fetch('/setVideoFps=' + fps)
                    .then(response => response.text())
                    .then(data => console.log('Video FPS Success:', data))
                    .catch((error) => console.error('Video FPS Error:', error));
            };

            // Handle Bitrate slider change
            bitrateSlider.oninput = function() {
                bitrateValue.innerHTML = "Bitrate: " + this.value;
                const bitrate = this.value;
                fetch('/setBitrate=' + bitrate)
                    .then(response => response.text())
                    .then(data => console.log('Bitrate Success:', data))
                    .catch((error) => console.error('Bitrate Error:', error));
            };

            // Handle IR ON button click
            irOnButton.onclick = function() {
                fetch('/setIR=ON')
                    .then(response => response.text())
                    .then(data => console.log('IR ON Success:', data))
                    .catch((error) => console.error('IR ON Error:', error));
            };

            // Handle IR OFF button click
            irOffButton.onclick = function() {
                fetch('/setIR=OFF')
                    .then(response => response.text())
                    .then(data => console.log('IR OFF Success:', data))
                    .catch((error) => console.error('IR OFF Error:', error));
            };

            // Handle Restart button click
            restartButton.onclick = function() {
                fetch('/restart=1')
                    .then(response => response.text())
                    .then(data => console.log('Restart Success:', data))
                    .catch((error) => console.error('Restart Error:', error));
            };
        </script>
    </body>
    </html>
    )";

    // Send the HTML response to the client
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += html_content;

    send(client_fd, response.c_str(), response.length(), 0);
}

void start_server(short port) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Error creating socket!" << std::endl;
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Error binding socket!" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        std::cerr << "Error listening on socket!" << std::endl;
        exit(EXIT_FAILURE);
    }

            std::string sensor_fps_command = "echo setfps 0 5 > /proc/mi_modules/mi_sensor/mi_sensor0";
            std::cout << "Starting sensor with: " << sensor_fps_command << std::endl;
            std::system(sensor_fps_command.c_str());

    std::cout << "Server listening on port " << port << "..." << std::endl;

    while (true) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            std::cerr << "Error accepting connection!" << std::endl;
            continue;
        }

        char buffer[1024];
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            std::string request(buffer);

            // Handle the requests for FPS, Resolution, Bitrate, IR, etc.
            handle_combined_fps_request(request);
            handle_resolution_request(request);
            handle_bitrate_request(request);
            handle_ir_request(request);
            handle_restart_request(request);
        }

        send_html_response(client_fd);
        close(client_fd);
    }

    close(server_fd);
}

int main() {
    short port = 2221;
    start_server(port);
    return 0;
}
