#include "vid_render/TVidRender.hpp"

#include "conf/version.hpp"

#include "utils/TDeduction.hpp"
#include "utils/TLog.hpp"

#include <stdexcept>
#include <string_view>

#define T_LOG_TAG_IMG ">>Video Render<< "

using namespace std;
using namespace std::string_view_literals;
namespace gentau {
TVidRender::TVidRender(const char* file_path) : naluBuffer(0)
{
	// Check in compile-time
	if constexpr (conf::TDebugMode) {
		// We don't need a buffer for file playback
		pipeline = gst_pipeline_new("pipeline");
		src      = gst_element_factory_make("filesrc", "src");
		parser   = gst_element_factory_make("h265parse", "parser");
		decoder  = gst_element_factory_make("decodebin", "decoder");
		vconv    = gst_element_factory_make("videoconvert", "vconv");
		conv     = gst_element_factory_make("glupload", "conv");
		sink     = gst_element_factory_make("qml6glsink", "sink");

		if (anyFalse(pipeline, src, parser, decoder, vconv, conv, sink)) {
			constexpr auto errMsg = "Failed to create all Gstreamer elements."sv;
			tImgTransLogCritical("{}", errMsg);
			throw std::runtime_error(errMsg.data());
		}

		gst_bin_add_many(GST_BIN(pipeline), src, parser, decoder, vconv, conv, sink, nullptr);

		if (anyFalse(
				gst_element_link_many(vconv, conv, sink, nullptr),
				gst_element_link_many(src, parser, decoder, nullptr)
			)) {
			constexpr auto errMsg =
				"Failed to link GStreamer elements."sv;  // string_view 在编译期被构造和分配空间
			tImgTransLogCritical("{}", errMsg);
			throw std::runtime_error(
				errMsg.data()
			);  // string_view 仅在从字符串字面量构建时，才保证以 \0 结尾
		}

		g_object_set(sink, "sync", FALSE, nullptr);
		g_object_set(src, "location", file_path, nullptr);

	} else {
		constexpr auto errMsg =
			"Calling TVidRender(const char* file_path) is not supported in release builds."sv;
		tImgTransLogCritical("{}", errMsg);
		throw std::runtime_error(errMsg.data());
	}
}
}  // namespace gentau