#include "vid_render/TVidRender.hpp"

#include "conf/version.hpp"

#include "utils/TDeduction.hpp"
#include "utils/TLog.hpp"
#include "utils/TLogical.hpp"

#include <gst/app/app.h>

#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

#define T_LOG_TAG_IMG       "[Video Render] "
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
	g_autoptr(GstPad) sink_pad = gst_element_get_static_pad(self->uploader, "sink");

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

GstElement* TVidRender::choosePrefDecoder(bool& isDynamic)
{
	vector<const gchar*> candidates = {
#ifdef __linux__
		"nvh265dec",     // Nvidia
		"vah265dec",     // VA-API (Intel/AMD)(high priority in gstreamer)
		"vaapih265dec",  // VA-API
#elif defined(WIN32)
		"nvh265dec",     // Nvidia
		"d3d12h265dec",  // D3D12
		"d3d11h265dec",  // D3D11
		"qsvh265dec",    // QuickSync (Intel)
#elif defined(__APPLE__)
		"vtdec_h265_hw",  // VideoToolbox H.265 hardware only
		"vtdec_hw",       // General VideoToolbox hardware decoder
		"vtdec_h265",     // VideoToolbox H.265 software
		"vtdec",
#endif
		"avdec_h265",  // FFMPEG software decoder as fallback
	};

	for (auto name : candidates) {
		GstElementFactory* factory = gst_element_factory_find(name);
		if (factory) {
			// Further verify for gstreamer plugin blacklist
			GstElement* element = gst_element_factory_create(factory, "decoder");
			gst_object_unref(GST_OBJECT(factory));

			if (element) {
				tImgTransLogTrace("Selected H.265 Decoder: '{}'", name);
				isDynamic = false;
				return element;
			}
		}
	}

	string warnMsg = "No preferred hardware decoder found, falling back to software decodebin. Performance may be suboptimal.";	
	tImgTransLogWarn("{}", warnMsg);

	isDynamic = true;
	return gst_element_factory_make("decodebin", "decoder");
}

bool TVidRender::initBusThread()
{
	if (!pipeline) {
		constexpr auto errMsg = "Pipeline is not initialized, cannot start bus thread."sv;
		tImgTransLogCritical("{}", errMsg);
		throw std::runtime_error(errMsg.data());
	}

	busThread = jthread(
		[this](stop_token sToken) {
			GstBus* bus = gst_element_get_bus(pipeline);
			while (!sToken.stop_requested()) {
				GstMessage* msg = gst_bus_timed_pop(bus, GST_MSECOND * 100);
				if (msg) {
					GError* err;
					gchar*  debug;
					switch (GST_MESSAGE_TYPE(msg)) {
						case GST_MESSAGE_ERROR:
							gst_message_parse_error(msg, &err, &debug);
							tImgTransLogError(
								"\n=========== ERROR ==========\nError: {}\nDebug: {}\n============================\n",
								err->message,
								debug
							);
							g_error_free(err);
							g_free(debug);
							break;
						case GST_MESSAGE_WARNING:
							gst_message_parse_warning(msg, &err, &debug);
							tImgTransLogWarn("Warning: {}\nDebug: {}", err->message, debug);
							g_error_free(err);
							g_free(debug);
							break;
						default:
							break;
					}
					gst_message_unref(msg);
				}
			}
			gst_object_unref(bus);
		}
	);
}

bool TVidRender::initPipeElements(bool useFileSrc, const char* file_path)
{
	bool         linkDynamic = false;
	const gchar* srcType     = useFileSrc ? "filesrc" : "appsrc";

	pipeline       = gst_pipeline_new("pipeline");
	src            = gst_element_factory_make(srcType, "src");
	parser         = gst_element_factory_make("h265parse", "parser");
	bufferQueue    = gst_element_factory_make("queue", "bufferQueue");
	decoder        = choosePrefDecoder(linkDynamic);
	leakyQueue     = gst_element_factory_make("queue", "leakyQueue");
	colorConv      = gst_element_factory_make("glcolorconvert", "colorConv");
	uploader       = gst_element_factory_make("glupload", "uploader");
	sinkCapsFilter = gst_element_factory_make("capsfilter", "sinkCapsFilter");
	sink           = gst_element_factory_make("qml6glsink", "sink");

	if (anyFalse(
			pipeline,
			src,
			parser,
			decoder,
			bufferQueue,
			uploader,
			colorConv,
			leakyQueue,
			sinkCapsFilter,
			sink
		)) {
		for (auto elem : { pipeline,
						   src,
						   parser,
						   decoder,
						   bufferQueue,
						   uploader,
						   colorConv,
						   leakyQueue,
						   sinkCapsFilter,
						   sink }) {
			if (elem) { gst_object_unref(elem); }
		}

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
		uploader,
		colorConv,
		leakyQueue,
		sinkCapsFilter,
		sink,
		nullptr
	);

	constexpr auto errMsg =
		"Failed to link GStreamer elements."sv;  // string_view 在编译期被构造和分配空间

	if (linkDynamic) {
		if (anyFalse(
				gst_element_link_many(
					uploader, colorConv, sinkCapsFilter, leakyQueue, sink, nullptr
				),
				gst_element_link_many(src, parser, bufferQueue, decoder, nullptr)
			)) {
			gst_object_unref(pipeline);
			tImgTransLogCritical("{}", errMsg);
			throw std::runtime_error(
				errMsg.data()
			);  // string_view 仅在从字符串字面量构建时，才保证以 \0 结尾
		}

		g_signal_connect(decoder, "pad-added", G_CALLBACK(onDecoderPadAdded), this);
		tImgTransLogTrace("Decoder will be linked dynamically");
	} else {
		if (!gst_element_link_many(
				src,
				parser,
				bufferQueue,
				decoder,
				uploader,
				colorConv,
				sinkCapsFilter,
				leakyQueue,
				sink,
				nullptr
			)) {
			gst_object_unref(pipeline);
			tImgTransLogCritical("{}", errMsg);
			throw std::runtime_error(errMsg.data());
		}
		tImgTransLogTrace("Decoder will be linked statically");
	}

	g_object_set(sink, "sync", FALSE, "max-lateness", MAX_RENDER_DELAY, nullptr);
	if (useFileSrc) {
		g_object_set(src, "location", file_path, nullptr);
	} else {
		// 指定appsrc的caps属性('video/x-265')可能会导致h265parse在解析时更严格，从而导致播放卡死，因此暂不指定caps
		g_object_set(
			src,
			"is-live",
			TRUE,
			"min-latency",
			0,  // No latency, push frames as soon as possible
			"max-latency",
			-1,  // Send at best effort
			"max-bytes",
			(guint64)(80 * 1000),  // Serial baud rate at 912600 (8N1), bandwidth is about 90KB/s
			"format",
			GST_FORMAT_BYTES,
			"stream-type",
			GST_APP_STREAM_TYPE_STREAM,
			"emit-signals",
			FALSE,
			"block",
			FALSE,
			nullptr
		);
	}

	g_object_set(parser, "config-interval", -1, nullptr);
	g_object_set(bufferQueue, "max-size-buffers", 2, "leaky", 0, nullptr);
	g_object_set(
		leakyQueue,
		"max-size-buffers",
		1,  // Only keep the last frame
		"max-size-bytes",
		0,  // Disabled
		"max-size-time",
		0,  // Disabled
		"leaky",
		2,  // downstream
		nullptr
	);

	// Caps string ref: https://fossies.org/linux/gstreamer/tests/check/gst/gstcaps.c
	// Line 148:156 'non_simple_caps_string' and Line 216:228
	const gchar* sinkCapStr =
		"video/x-raw(memory:GLMemory), "
#ifdef __linux__
		"format=(string){NV12, RGBA, BGRA}, "
#elif defined(__APPLE__)
		"format=(string){RGBA, BGRA}, "
#endif
		"texture-target=(string)2D";
	g_autoptr(GstCaps) caps =
		gst_caps_from_string(sinkCapStr);  // This API's behavior is not stable under 1.20
	g_object_set(sinkCapsFilter, "caps", caps, nullptr);

	return true;
}

TVidRender::TVidRender(const char* file_path)
{
	// Check in compile-time
	if constexpr (conf::TDebugMode) {
		initPipeElements(true, file_path);
	} else {
		constexpr auto errMsg =
			"Calling TVidRender(const char* file_path) is not supported in release builds."sv;
		tImgTransLogCritical("{}", errMsg);
		throw std::runtime_error(errMsg.data());
	}
}

TVidRender::TVidRender()
{
	initPipeElements(false);
}

TVidRender::~TVidRender()
{
	if (pipeline) {
		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_object_unref(pipeline);
		pipeline = nullptr;
	}

	isRunning.store(false);
}

bool TVidRender::tryPushFrame(TVidRender::FramePtr frame)
{
	if (!isRunning.load()) {
		tImgTransLogWarn("Attempted to push frame while renderer is not running.");
		return false;
	}

	// tImgTransLogDebug("Start to push");

	auto raw_vec_ptr = frame.release();

	// 2. 使用 wrapped_full，并提供一个自定义的释放回调
	GstBuffer* buffer = gst_buffer_new_wrapped_full(
		GST_MEMORY_FLAG_READONLY,
		raw_vec_ptr->data(),  // 内存地址
		raw_vec_ptr->size(),  // 内存大小
		0,                    // 偏移
		raw_vec_ptr->size(),  // 实际大小
		raw_vec_ptr,          // user_data: 传入 vector 指针
		[](gpointer data) {
			auto p = static_cast<std::vector<u8>*>(data);
			delete p;
		}
	);

	if (buffer) {
		auto ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
		if (ret != GST_FLOW_OK) {
			tImgTransLogError("Failed to push buffer to appsrc, error: {}", gst_flow_get_name(ret));
			return false;
		}
	}

	// tImgTransLogDebug("Push is over");
	return true;
}

bool TVidRender::play()
{
	GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		tImgTransLogError("Failed to set pipeline to PLAYING state.");
		return false;
	}
	tImgTransLogInfo("Pipeline set to PLAYING state.");
	isRunning.store(true);
	return true;
}

void TVidRender::linkSinkWidget(QQuickItem* widget)
{
	g_object_set(sink, "widget", widget, nullptr);
}
}  // namespace gentau