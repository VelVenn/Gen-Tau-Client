#include "vid_render/TVidRender.hpp"

#include "conf/version.hpp"

#include "utils/TDeduction.hpp"
#include "utils/TLog.hpp"

#include <glibconfig.h>
#include <gst/gstquery.h>
#include <stdexcept>
#include <string_view>

#define T_LOG_TAG_IMG       ">>Video Render<< "
#define RENDER_WAIT_FOREVER (0 && GEN_TAU_DEBUG)

using namespace std;
using namespace std::string_view_literals;
namespace gentau {
#if RENDER_WAIT_FOREVER == 1
constexpr gint64 MAX_RENDER_DELAY = -1;
#else
constexpr auto MAX_RENDER_DELAY = 20 * GST_MSECOND;
#endif

void TVidRender::onDecoderPadAdded(GstElement* decoder, GstPad* new_pad, gpointer user_data)
{
	TVidRender* self           = static_cast<TVidRender*>(user_data);
	g_autoptr(GstPad) sink_pad = gst_element_get_static_pad(self->vconv, "sink");
}

TVidRender::TVidRender(const char* file_path) : naluBuffer(0)
{
	// Check in compile-time
	if constexpr (conf::TDebugMode) {
		// We don't need a buffer for file playback
		pipeline = gst_pipeline_new("pipeline");
		src      = gst_element_factory_make("filesrc", "src");
		parser   = gst_element_factory_make("h265parse", "parser");
		queue    = gst_element_factory_make("queue", "queue");
		decoder  = gst_element_factory_make("decodebin", "decoder");
		vconv    = gst_element_factory_make("videoconvert", "vconv");
		conv     = gst_element_factory_make("glupload", "conv");
		sink     = gst_element_factory_make("qml6glsink", "sink");

		if (anyFalse(pipeline, src, parser, queue, decoder, vconv, conv, sink)) {
			constexpr auto errMsg = "Failed to create all Gstreamer elements."sv;
			tImgTransLogCritical("{}", errMsg);
			throw std::runtime_error(errMsg.data());
		}

		gst_bin_add_many(
			GST_BIN(pipeline), src, parser, queue, decoder, vconv, conv, sink, nullptr
		);

		if (anyFalse(
				gst_element_link_many(vconv, conv, sink, nullptr),
				gst_element_link_many(src, parser, queue, decoder, nullptr)
			)) {
			constexpr auto errMsg =
				"Failed to link GStreamer elements."sv;  // string_view 在编译期被构造和分配空间
			tImgTransLogCritical("{}", errMsg);
			throw std::runtime_error(
				errMsg.data()
			);  // string_view 仅在从字符串字面量构建时，才保证以 \0 结尾
		}

		g_object_set(sink, "sync", FALSE, "max-lateness", MAX_RENDER_DELAY, nullptr);
		g_object_set(src, "location", file_path, nullptr);
		g_object_set(parser, "config-interval", -1, nullptr);
		g_object_set(
			queue,
			"max-size-buffers",
			2,
			"max-size-bytes",
			0,  // Disabled
			"max-size-time",
			0,  // Disabled
			"leaky",
			2,  // downstream
			nullptr
		);

		g_signal_connect(decoder, "pad-added", G_CALLBACK(onDecoderPadAdded), this);

	} else {
		constexpr auto errMsg =
			"Calling TVidRender(const char* file_path) is not supported in release builds."sv;
		tImgTransLogCritical("{}", errMsg);
		throw std::runtime_error(errMsg.data());
	}
}
}  // namespace gentau