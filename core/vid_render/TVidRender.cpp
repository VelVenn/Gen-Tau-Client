#include "vid_render/TVidRender.hpp"

#include "conf/version.hpp"

#include "utils/TLogical.hpp"
#include "utils/TDeduction.hpp"
#include "utils/TLog.hpp"

#include <stdexcept>
#include <string>
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

// Refcounting of Gstreamer objects
// Original: https://gstreamer.freedesktop.org/documentation/additional/design/MT-refcounting.html?gi-language=c#refcounting1

// All new objects created have the FLOATING flag set. This means that the object 
// is not owned or managed yet by anybody other than the one holding a reference 
// to the object. The object in this state has a reference count of 1.

// Various object methods can take ownership of another object, this means that 
// after calling a method on object A with an object B as an argument, the object 
// B is made sole property of object A. This means that after the method call you 
// are not allowed to access the object anymore unless you keep an extra reference 
// to the object. An example of such a method is the _bin_add() method. As soon as 
// this function is called in a Bin, the element passed as an argument is owned by 
// the bin and you are not allowed to access it anymore without taking a _ref() 
// before adding it to the bin. The reason being that after the _bin_add() call 
// disposing the bin also destroys the element.

// Taking ownership of an object happens through the process of "sinking" the object. 
// The _sink() method on an object will decrease the refcount of the object if the 
// FLOATING flag is set. The act of taking ownership of an object is then performed 
// as a _ref() followed by a _sink() call on the object.

// The float/sink process is very useful when initializing elements that will then be 
// placed under control of a parent. The floating ref keeps the object alive until it 
// is parented, and once the object is parented you can forget about it.

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