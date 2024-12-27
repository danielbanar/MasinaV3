# Define compiler and flags
CXX = arm-linux-gnueabihf-g++
CXXFLAGS = -march=armv7-a -mfpu=neon -mfloat-abi=hard -O2 -flto -fdata-sections -ffunction-sections -mtune=cortex-a9
PTHREAD = -pthread

# Targets
TARGETS = masinaclient masinawebserver

# Default target: build both client and masinawebserver
all: $(TARGETS)

# Build the client executable
masinaclient: client.cpp utils.cpp
	$(CXX) $(CXXFLAGS) client.cpp utils.cpp -o masinaclient $(PTHREAD) -Wl,--gc-sections

# Build the masinawebserver executable
masinawebserver: masinawebserver.cpp
	$(CXX) $(CXXFLAGS) masinawebserver.cpp -o masinawebserver -Wl,--gc-sections

# Clean target: removes object files and executables
clean:
	rm -f $(TARGETS)

# Strip executable to remove debugging symbols (if you want to further reduce size)
strip: $(TARGETS)
	strip masinaclient masinawebserver

.PHONY: all clean strip
