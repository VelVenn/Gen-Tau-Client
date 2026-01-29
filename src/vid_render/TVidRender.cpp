#include "vid_render/TVidRender.hpp"

#include "conf/version.hpp"

#include <stdexcept>
#include <string>

using namespace std;

namespace gentau {
TVidRender::TVidRender(const char* file_path) : frameBuffer(0), isRunning(false)
{
    if(!conf::TDebugMode) {
        throw std::runtime_error("Video rendering is only available in debug builds.");
    }

    // We don't need a buffer for file playback
	pipeline = gst_pipeline_new("pipeline");
	src      = gst_element_factory_make("filesrc", "src");
	parser   = gst_element_factory_make("h265parse", "parser");
	decoder  = gst_element_factory_make("decodebin", "decoder");
	vconv    = gst_element_factory_make("videoconvert", "vconv");
	conv     = gst_element_factory_make("glupload", "conv");
	sink     = gst_element_factory_make("qml6glsink", "sink");

	if(!pipeline || !src || !parser || !decoder || !vconv || !conv || !sink) {
		throw std::runtime_error("Failed to create all GStreamer elements for TVidRender.");
	}

	gst_bin_add_many(
		GST_BIN(pipeline), src, parser, decoder, vconv, conv, sink, NULL
	);

}
}  // namespace gentau