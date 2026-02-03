#include "vid_render/TVidRender.hpp"
#include <glib-object.h>
#include <gst/gstcaps.h>

#include "conf/version.hpp"

#include "utils/TDeduction.hpp"
#include "utils/TLog.hpp"
#include "utils/TLogical.hpp"

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
	g_autoptr(GstPad) sink_pad = gst_element_get_static_pad(self->leakyQueue, "sink");

	GstPadLinkReturn ret;
	g_autoptr(GstCaps) new_pad_caps = nullptr;
	GstStructure* new_pad_struct    = nullptr;
	const gchar*  new_pad_type      = nullptr;

	tImgTransLogTrace(
		"Recieved new pad '{}' from decoder '{}'", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(decoder)
	);

	if (gst_pad_is_linked(sink_pad)) {
		tImgTransLogTrace("Sink pad already linked, ignored");
		return;
	}

	new_pad_caps   = gst_pad_get_current_caps(new_pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type   = gst_structure_get_name(new_pad_struct);

	if (g_str_has_prefix(new_pad_type, "video/x-raw")) {
		ret = gst_pad_link(new_pad, sink_pad);
		if (GST_PAD_LINK_FAILED(ret)) {
			tImgTransLogError(
				"Type '{}' pad link failed with error '{}'",
				new_pad_type,
				gst_pad_link_get_name(ret)
			);
		} else {
			tImgTransLogInfo("Type '{}' pad linked successfully", new_pad_type);
		}
	} else {
		tImgTransLogTrace("Type '{}' pad ignored", new_pad_type);
	}
}

TVidRender::TVidRender(const char* file_path) : naluBuffer(0)
{
	// Check in compile-time
	if constexpr (conf::TDebugMode) {
		// We don't need a buffer for file playback
		pipeline    = gst_pipeline_new("pipeline");
		src         = gst_element_factory_make("filesrc", "src");
		parser      = gst_element_factory_make("h265parse", "parser");
		bufferQueue = gst_element_factory_make("queue", "bufferQueue");
		decoder     = gst_element_factory_make("decodebin", "decoder");
		leakyQueue  = gst_element_factory_make("queue", "leakyQueue");
		colorConv   = gst_element_factory_make("glcolorconvert", "colorConv");
		uploader    = gst_element_factory_make("glupload", "uploader");
		sink        = gst_element_factory_make("qml6glsink", "sink");

		if (anyFalse(
				pipeline, src, parser, decoder, bufferQueue, leakyQueue, uploader, colorConv, sink
			)) {
			constexpr auto errMsg = "Failed to create all Gstreamer elements."sv;
			tImgTransLogCritical("{}", errMsg);
			throw std::runtime_error(errMsg.data());
		}

		gst_bin_add_many(
			GST_BIN(pipeline),
			src,
			parser,
			decoder,
			bufferQueue,
			leakyQueue,
			uploader,
			colorConv,
			sink,
			nullptr
		);

		if (anyFalse(
				gst_element_link_many(leakyQueue, uploader, colorConv, sink, nullptr),
				gst_element_link_many(src, parser, bufferQueue, decoder, nullptr)
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
		g_object_set(bufferQueue, "max-size-buffers", 20, "leaky", 0, nullptr);
		g_object_set(
			leakyQueue,
			"max-size-buffers",
			2,
			"max-size-bytes",
			0,
			"max-size-time",
			0,
			"leaky",
			2,
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

TVidRender::~TVidRender()
{
	if (pipeline) {
		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_object_unref(pipeline);
		pipeline = nullptr;
	}
}

bool TVidRender::play()
{
	GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		tImgTransLogError("Failed to set pipeline to PLAYING state.");
		return false;
	}
	tImgTransLogInfo("Pipeline set to PLAYING state.");
	return true;
}

void TVidRender::linkWidget(QQuickItem* widget)
{
	g_object_set(sink, "widget", widget, nullptr);
}
}  // namespace gentau