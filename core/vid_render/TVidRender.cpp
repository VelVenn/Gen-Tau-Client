#include "vid_render/TVidRender.hpp"

#include "conf/version.hpp"

#include "utils/TLog.hpp"
#include "utils/TLogical.hpp"

#include <gst/app/app.h>

#include <exception>
#include <future>
#include <stdexcept>
#include <stop_token>
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
constexpr auto MAX_RENDER_DELAY = 25 * GST_MSECOND;
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
	TVidRender* self               = static_cast<TVidRender*>(user_data);
	g_autoptr(GstElement) uploader = gst_bin_get_by_name(GST_BIN(self->fixedPipe), "uploader");
	g_autoptr(GstPad) sink_pad     = gst_element_get_static_pad(uploader, "sink");

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
		"vtdec_hw",       // General VideoToolbox hardware decoder
		"vtdec_h265_hw",  // VideoToolbox H.265 hardware only
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

	tImgTransLogWarn("No preferred decoder found, falling back to software decodebin.");

	isDynamic = true;
	return gst_element_factory_make("decodebin", "decoder");
}

bool TVidRender::initBusThread()
{
	if (!pipeline()) {
		constexpr auto errMsg = "Pipeline is not initialized, cannot start bus thread."sv;
		tImgTransLogCritical("{}", errMsg);
		throw std::runtime_error(errMsg.data());
	}

	promise<void> threadErrPassThru;
	auto          passThruFuture = threadErrPassThru.get_future();

	// lamba捕获变量时会默认将其视为const成员
	busThread = jthread([this, passThru = std::move(threadErrPassThru)](stop_token sToken) mutable {
		g_autoptr(GstBus) bus = gst_element_get_bus(fixedPipe);

		if (!bus) {
			constexpr auto errMsg = "Failed to get bus from pipeline."sv;
			tImgTransLogCritical("{}", errMsg);
			passThru.set_exception(make_exception_ptr(runtime_error(errMsg.data())));
			return;
		}

		passThru.set_value();  // Notify main thread init success

		auto issueParser = [this](GstMessage* msg, bool asPipeErr) {
			g_autoptr(GError) err       = nullptr;
			g_autofree gchar* debugInfo = nullptr;
			IssueType         iType     = IssueType::UNKNOWN;
			string            errSrc    = msg->src ? GST_ELEMENT_NAME(msg->src) : "Unknown";

			gst_message_parse_error(msg, &err, &debugInfo);

			if (err) {
				auto domain = err->domain;

				if (domain == GST_CORE_ERROR || domain == GST_LIBRARY_ERROR) {
					iType = IssueType::PIPELINE_INTERNAL;
				} else if (domain == GST_STREAM_ERROR) {
					iType = IssueType::PIPELINE_STREAM;
				} else if (domain == GST_RESOURCE_ERROR) {
					iType = IssueType::PIPELINE_RESOURCE;
				} else {
					iType = IssueType::PIPELINE_OTHER;
				}

				if (asPipeErr) {
					signals.onPipeError(iType, errSrc, err->message, debugInfo ? debugInfo : "");

					tImgTransLogError(
						"Render Engine error: {} | Debug info : {}",
						err->message,
						debugInfo ? debugInfo : "(none)"
					);
				} else {
					signals.onPipeWarn(iType, errSrc, err->message, debugInfo ? debugInfo : "");

					tImgTransLogWarn(
						"Render Engine warning: {} | Debug info : {}",
						err->message,
						debugInfo ? debugInfo : "(none)"
					);
				}
			}
		};

		while (!sToken.stop_requested()) {
			g_autoptr(GstMessage) msg = gst_bus_timed_pop_filtered(
				bus,
				100 * GST_MSECOND,
				static_cast<GstMessageType>(
					GST_MESSAGE_EOS | GST_MESSAGE_ERROR | GST_MESSAGE_WARNING |
					GST_MESSAGE_STATE_CHANGED
				)
			);

			if (!msg) { continue; }

			switch (GST_MESSAGE_TYPE(msg)) {
				case GST_MESSAGE_EOS: {
					tImgTransLogInfo("End of stream reached.");
					signals.onEOS();
					break;
				}
				case GST_MESSAGE_ERROR: {
					issueParser(msg, true);
					break;
				}
				case GST_MESSAGE_WARNING: {
					issueParser(msg, false);
					break;
				}
				case GST_MESSAGE_STATE_CHANGED: {
					if (GST_MESSAGE_SRC(msg) == GST_OBJECT(fixedPipe)) {
						GstState oldState, newState;
						gst_message_parse_state_changed(msg, &oldState, &newState, nullptr);
						tImgTransLogInfo(
							"Pipeline state changed from '{}' to '{}'",
							gst_element_state_get_name(oldState),
							gst_element_state_get_name(newState)
						);
						signals.onStateChanged(convGstState(oldState), convGstState(newState));
					}

					break;
				}
				default:
					tImgTransLogWarn(
						"Something weird happened, it should never goto busThread's default branch "
						"..."
					);  // Should never reach here
			}
		}
	});

	try {
		passThruFuture.get();
	} catch (const exception& e) {
		throw;  // Rethrow to main thread
	}

	tImgTransLogInfo("Bus thread started successfully.");
	return true;
}

bool TVidRender::initPipeElements(bool useFileSrc, const char* file_path)
{
	bool         linkDynamic = false;
	const gchar* srcType     = useFileSrc ? "filesrc" : "appsrc";

	fixedPipe                 = gst_pipeline_new("pipeline");
	fixedSrc                  = gst_element_factory_make(srcType, "src");
	ElemRawPtr parser         = gst_element_factory_make("h265parse", "parser");
	ElemRawPtr bufferQueue    = gst_element_factory_make("queue", "bufferQueue");
	ElemRawPtr decoder        = choosePrefDecoder(linkDynamic);
	ElemRawPtr leakyQueue     = gst_element_factory_make("queue", "leakyQueue");
	ElemRawPtr colorConv      = gst_element_factory_make("glcolorconvert", "colorConv");
	ElemRawPtr uploader       = gst_element_factory_make("glupload", "uploader");
	ElemRawPtr sinkCapsFilter = gst_element_factory_make("capsfilter", "sinkCapsFilter");
	fixedSink                 = gst_element_factory_make("qml6glsink", "sink");

	if (anyFalse(
			fixedPipe,
			fixedSrc,
			parser,
			decoder,
			bufferQueue,
			uploader,
			colorConv,
			leakyQueue,
			sinkCapsFilter,
			fixedSink
		)) {
		for (auto elem : { fixedPipe,
						   fixedSrc,
						   parser,
						   decoder,
						   bufferQueue,
						   uploader,
						   colorConv,
						   leakyQueue,
						   sinkCapsFilter,
						   fixedSink }) {
			if (elem) { gst_object_unref(elem); }
		}

		constexpr auto errMsg = "Failed to create all Gstreamer elements."sv;
		tImgTransLogCritical("{}", errMsg);
		throw std::runtime_error(errMsg.data());
	}

	gst_bin_add_many(
		GST_BIN(fixedPipe),
		fixedSrc,
		parser,
		decoder,
		bufferQueue,
		uploader,
		colorConv,
		leakyQueue,
		sinkCapsFilter,
		fixedSink,
		nullptr
	);

	constexpr auto errMsg =
		"Failed to link GStreamer elements."sv;  // string_view 在编译期被构造和分配空间

	if (linkDynamic) {
		if (anyFalse(
				gst_element_link_many(
					uploader, colorConv, sinkCapsFilter, leakyQueue, fixedSink, nullptr
				),
				gst_element_link_many(fixedSrc, parser, bufferQueue, decoder, nullptr)
			)) {
			gst_object_unref(fixedPipe);
			tImgTransLogCritical("{}", errMsg);
			throw std::runtime_error(
				errMsg.data()
			);  // string_view 仅在从字符串字面量构建时，才保证以 \0 结尾
		}

		g_signal_connect(decoder, "pad-added", G_CALLBACK(onDecoderPadAdded), this);
		tImgTransLogTrace("Decoder will be linked dynamically");
	} else {
		if (!gst_element_link_many(
				fixedSrc,
				parser,
				bufferQueue,
				decoder,
				uploader,
				colorConv,
				sinkCapsFilter,
				leakyQueue,
				fixedSink,
				nullptr
			)) {
			gst_object_unref(fixedPipe);
			tImgTransLogCritical("{}", errMsg);
			throw std::runtime_error(errMsg.data());
		}
		tImgTransLogTrace("Decoder will be linked statically");
	}

	g_object_set(fixedSink, "sync", FALSE, "max-lateness", MAX_RENDER_DELAY, nullptr);
	if (useFileSrc) {
		g_object_set(fixedSrc, "location", file_path, nullptr);
	} else {
		// 指定appsrc的caps属性('video/x-265')可能会导致h265parse在解析时更严格，从而导致播放卡死，因此暂不指定caps
		g_object_set(
			fixedSrc,
			"is-live",
			TRUE,
			"min-latency",
			0,  // No latency, push frames as soon as possible
			"max-latency",
			-1,  // Send at best effort
			"max-bytes",
			(guint64)(90 * 1000),  // Serial baud rate at 912600 (8N1), bandwidth is about 90KB/s
			"do-timestamp",
			TRUE,
			"format",
			GST_FORMAT_TIME,
			"stream-type",
			GST_APP_STREAM_TYPE_STREAM,
			"emit-signals",
			FALSE,
			"block",
			FALSE,
			nullptr
		);

		// 根据Gstreamer文档的建议，打timestamp和live-source通常要设置'format'为'GST_FORMAT_TIME'，
		// 但是考虑到发送端可能是透传，设置为'GST_FORMAT_BYTES'可能更合适。这个目前来看影响不大，如果后续
		// 发现问题再调整。

		// Using the appsrc
		// Ref: https://gstreamer.freedesktop.org/documentation/application-development/advanced/pipeline-manipulation.html?gi-language=c#inserting-data-with-appsrc
	}

	// 开启disable-passthrough会强制parse解析每一帧，理论上可以降低缺/错帧带来的影响，但也可能增加CPU负担，目前看来是否开启对管线本身对稳定性影响不大
	// config-interval最好设置为-1，让parse在遇到关键帧时重新配置(VPS, SPS, PPS)，这个选项对管线的稳定性与恢复能力影响较大
	g_object_set(parser, "config-interval", -1, "disable-passthrough", FALSE, nullptr);
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

	tImgTransLogInfo("Pipeline initialized successfully, ready to start bus thread.");

	bool res = initBusThread();

	return res;
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
	if (fixedPipe) {
		gst_element_set_state(fixedPipe, GST_STATE_NULL);
		gst_object_unref(fixedPipe);
		fixedPipe = nullptr;
	}
}

bool TVidRender::tryPushFrame(TVidRender::FramePtr frame)
{
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
		// Push may success at GST_STATE_PAUSED or GST_STATE_PLAYING
		auto ret = gst_app_src_push_buffer(GST_APP_SRC(fixedSrc), buffer);
		if (ret == GST_FLOW_OK) {
			lastPushSuccess.store(chrono::steady_clock::now());
			return true;
		}

		if (ret <= GST_FLOW_ERROR) {
			string errMsg = "Fatal error occured while trying to push frame buffer, flow return: " +
							string(gst_flow_get_name(ret));

			signals.onPipeError(IssueType::PUSH_FATAL, "appsrc", errMsg, "");
			tImgTransLogCritical("{}", errMsg);
		} else {
			tImgTransLogWarn(
				"Failed to push frame buffer, flow return: {}", gst_flow_get_name(ret)
			);
		}
	}

	return false;
}

bool TVidRender::play()
{
	if (!pipeline()) {
		tImgTransLogError("Play failed: Pipeline is not initialized.");
		return false;
	}

	GstStateChangeReturn ret = gst_element_set_state(fixedPipe, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		tImgTransLogError("Failed to set pipeline to PLAYING state.");
		return false;
	}
	tImgTransLogInfo("Pipeline set to PLAYING state.");
	return true;
}

bool TVidRender::pause()
{
	if (!pipeline()) {
		tImgTransLogError("Pause failed: Pipeline is not initialized.");
		return false;
	}

	GstStateChangeReturn ret = gst_element_set_state(fixedPipe, GST_STATE_PAUSED);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		tImgTransLogError("Failed to set pipeline to PAUSED state.");
		return false;
	}
	tImgTransLogInfo("Pipeline set to PAUSED state.");
	return true;
}

// reset() 和 stopPipeline() 是硬件资源级的重置，在MacOS上如果依赖vtdec_hw系列的硬件解码器，这两个API可能会导致严重错误 
// 仅保证在Linux系统下的稳定性，其他平台应谨慎使用
bool TVidRender::reset()
{
	if (!pipeline()) {
		tImgTransLogError("Reset failed: Pipeline is not initialized.");
		return false;
	}

	GstStateChangeReturn ret = gst_element_set_state(fixedPipe, GST_STATE_NULL);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		tImgTransLogError("Failed to reset pipeline");
		return false;
	}

	if (!play()) {
		tImgTransLogError("Failed to restart pipeline after reset");
		return false;
	}

	tImgTransLogInfo("Pipeline reset success");
	return true;
}

bool TVidRender::stopPipeline()
{
	if (!pipeline()) {
		tImgTransLogError("Stop failed: Pipeline is not initialized.");
		return false;
	}

	GstStateChangeReturn ret = gst_element_set_state(fixedPipe, GST_STATE_NULL);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		tImgTransLogError("Failed to stop pipeline");
		return false;
	}
	return true;
}

bool TVidRender::flush()
{
	if (!pipeline() || !src()) {
		tImgTransLogError("Flush failed: Pipeline is not initialized.");
		return false;
	}

	if (!gst_element_send_event(fixedSrc, gst_event_new_flush_start())) {
		tImgTransLogError("Failed to send FLUSH START event.");
		return false;
	}

	if (!gst_element_send_event(fixedSrc, gst_event_new_flush_stop(TRUE))) {
		tImgTransLogError("Failed to send FLUSH STOP event.");
		return false;
	}

#ifdef __APPLE__
	g_autoptr(GstCaps) currentCaps = nullptr;
	g_object_get(fixedSrc, "caps", &currentCaps, nullptr);

	// 重新设置它，这会欺骗解码器让它以为流变了，从而重置硬件会话
	if (currentCaps) { g_object_set(fixedSrc, "caps", currentCaps, nullptr); }
#endif

	tImgTransLogInfo("Pipeline flushed successfully.");
	return true;
}

TVidRender::StateType TVidRender::getCurrentState()
{
	GstState state;
	gst_element_get_state(fixedPipe, &state, nullptr, 0);
	return convGstState(state);
}

void TVidRender::linkSinkWidget(QQuickItem* widget)
{
	g_object_set(fixedSink, "widget", widget, nullptr);
}

void TVidRender::postTestError()
{
	if constexpr (conf::TDebugMode) {
		if (!fixedPipe) { return; }

		g_autoptr(GError) err =
			g_error_new(GST_CORE_ERROR, GST_CORE_ERROR_FAILED, "Artificial test error");

		GstMessage* msg =
			gst_message_new_error(GST_OBJECT(fixedPipe), err, "Debugging jthread effect");

		g_autoptr(GstBus) bus = gst_element_get_bus(fixedPipe);
		gst_bus_post(bus, msg);  // bus -> no transfer, msg -> transfer full
	} else {
		constexpr auto errMsg =
			"Calling TVidRender::postTestError() has no effect in non-debug builds."sv;
		tImgTransLogWarn("{}", errMsg);
	}
}
}  // namespace gentau