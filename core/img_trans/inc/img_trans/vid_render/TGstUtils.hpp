#pragma once

#include "utils/TTypeRedef.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

extern "C"
{
	struct _GstElement;
	struct _GstPad;
	typedef struct _GstElement GstElement;
	typedef struct _GstPad     GstPad;
	typedef void*              gpointer;
}

namespace gentau {

#define T_GST_BUS_SIGNAL(ClassName)                                                                \
  public:                                                                                          \
	TSignal<ClassName>                                                        onEOS;               \
	TSignal<ClassName, gst::IssueType, std::string, std::string, std::string> onPipeError;         \
	TSignal<ClassName, gst::IssueType, std::string, std::string, std::string> onPipeWarn;          \
	TSignal<ClassName, gst::StateType, gst::StateType>                        onStateChanged;      \
                                                                                                   \
  protected:                                                                                       \
	void emitOnEos()                                                                               \
	{                                                                                              \
		onEOS();                                                                                   \
	}                                                                                              \
	void emitOnPipeErr(gst::IssueType i, std::string src, std::string msg, std::string deb)        \
	{                                                                                              \
		onPipeError(i, src, msg, deb);                                                             \
	}                                                                                              \
	void emitOnPipeWarn(gst::IssueType i, std::string src, std::string msg, std::string deb)       \
	{                                                                                              \
		onPipeWarn(i, src, msg, deb);                                                              \
	}                                                                                              \
	void emitOnStateChanged(gst::StateType o, gst::StateType n)                                    \
	{                                                                                              \
		onStateChanged(o, n);                                                                      \
	}

namespace gst {
using Frame      = std::vector<u8>;
using FramePtr   = std::unique_ptr<Frame>;
using TimePoint  = std::chrono::steady_clock::time_point;
using ElemRawPtr = GstElement*;

enum class IssueType : u32
{
	UNKNOWN = 0,

	PIPELINE_INTERNAL,  // GStreamer pipeline internal error
	PIPELINE_RESOURCE,
	PIPELINE_STREAM,
	PIPELINE_OTHER,

	PUSH_FATAL,  // Issues relate to pushing frames
	PUSH_BUSY,

	GENERIC  // TSimpleVidRender self detected issue, not directly from GStreamer
};

enum class StateType : u8
{
	NULL_STATE = 0,
	READY,
	PAUSED,
	RUNNING
};

std::string_view getIssueTypeLiteral(IssueType type) noexcept;
std::string_view getStateLiteral(StateType state) noexcept;
}  // namespace gst
}  // namespace gentau