#include <gst/gst.h>

int main(int argc, char* argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the GStreamer pipeline
    GstElement* pipeline = gst_parse_launch("udpsrc port=2222 ! application/x-rtp,encoding-name=H264,payload=96 ! rtph264depay ! avdec_h264 ! videoscale ! video/x-raw,width=1280,height=960 ! fpsdisplaysink sync=false", NULL);

    // Start the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Run the main loop
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Cleanup
    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
