#!/bin/bash

# Generate a timestamp
timestamp=$(date +"%Y%m%d_%H%M%S")

# Define the filename with the timestamp
filename="/home/pi/videos/video_$timestamp.mkv"

# Run the GStreamer pipeline
gst-launch-1.0 rpicamsrc rotation=180 bitrate=0 quantisation-parameter=32 preview=false ! \
    video/x-h264,width=480,height=360,framerate=40/1 ! h264parse ! \
    tee name=t ! \
    queue ! rtph264pay config-interval=1 pt=96 ! udpsink host=your_ddns port=2222 \
    t. ! \
    queue ! matroskamux ! filesink location=$filename
