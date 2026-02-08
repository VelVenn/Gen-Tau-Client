#pragma once

#include "utils/TTypeRedef.hpp"

#include <gst/gst.h>

#include "readerwritercircularbuffer.h"

#include "sigslot/signal.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

class QQuickItem;

namespace gentau {
template<typename T>
struct IsSigslotSignal : std::false_type
{};

template<typename... Args>
struct IsSigslotSignal<sigslot::signal<Args...>> : std::true_type
{};

template<typename T>
concept SigslotSignal = IsSigslotSignal<T>::value;

class TVidRender : public std::enable_shared_from_this<TVidRender>
{
  public:
	using FramePtr   = std::unique_ptr<std::vector<u8>>;
	using ElemRawPtr = GstElement*;
	using TimePoint  = std::chrono::steady_clock::time_point;
	using Ptr        = std::shared_ptr<TVidRender>;

  public:
	enum class IssueType : u32
	{
		UNKNOWN = 0,

		PIPELINE_INTERNAL,  // GStreamer pipeline internal error
		PIPELINE_RESOURCE,
		PIPELINE_STREAM,
		PIPELINE_OTHER,

		PUSH_FATAL,  // Issues relate to pushing frames
		PUSH_BUSY,

		GENERIC  // TVidRender self detected issue, not directly from GStreamer
	};

	static std::string_view getIssueTypeLiteral(IssueType type) noexcept
	{
		switch (type) {
			case IssueType::UNKNOWN:
				return "UNKNOWN";
			case IssueType::PIPELINE_INTERNAL:
				return "PIPELINE INTERNAL";
			case IssueType::PIPELINE_RESOURCE:
				return "PIPELINE RESOURCE";
			case IssueType::PIPELINE_STREAM:
				return "PIPELINE STREAM";
			case IssueType::PIPELINE_OTHER:
				return "PIPELINE OTHER";
			case IssueType::PUSH_FATAL:
				return "PUSH FATAL";
			case IssueType::PUSH_BUSY:
				return "PUSH BUSY";
			case IssueType::GENERIC:
				return "GENERIC";
			default:
				return "UNDEFINED";
		}
	}

	enum class StateType : u8
	{
		NULL_STATE = 0,
		READY,
		PAUSED,
		RUNNING
	};

	static StateType convGstState(GstState state) noexcept
	{
		switch (state) {
			case GST_STATE_NULL:
				return StateType::NULL_STATE;
			case GST_STATE_READY:
				return StateType::READY;
			case GST_STATE_PAUSED:
				return StateType::PAUSED;
			case GST_STATE_PLAYING:
				return StateType::RUNNING;
			default:
				return StateType::NULL_STATE;
		}
	}

	static std::string_view getStateLiteral(StateType state) noexcept
	{
		switch (state) {
			case StateType::NULL_STATE:
				return "NULL STATE";
			case StateType::READY:
				return "READY";
			case StateType::PAUSED:
				return "PAUSED";
			case StateType::RUNNING:
				return "RUNNING";
			default:
				return "UNDEFINED";
		}
	}

  public:
	template<SigslotSignal T>
	class SignalWrapper
	{
	  private:
		T& signal;

	  public:
		template<typename... Args>
		auto connect(Args&&... args)
		{
			return signal.connect(std::forward<Args>(args)...);
		}

		SignalWrapper(T& sig) : signal(sig) {}
		~SignalWrapper() = default;

		SignalWrapper(const SignalWrapper&)            = delete;  // Forbid copy or move
		SignalWrapper& operator=(const SignalWrapper&) = delete;
		SignalWrapper(SignalWrapper&&)                 = delete;
		SignalWrapper& operator=(SignalWrapper&&)      = delete;
	};

	struct Signals
	{
		sigslot::signal<>                                                 onEOS;
		sigslot::signal<IssueType, std::string, std::string, std::string> onPipeError;
		sigslot::signal<IssueType, std::string, std::string, std::string> onPipeWarn;
		sigslot::signal<StateType, StateType>                             onStateChanged;
	};

	struct SignalView
	{
		SignalWrapper<decltype(Signals::onEOS)>          onEOS;
		SignalWrapper<decltype(Signals::onPipeError)>    onPipeError;
		SignalWrapper<decltype(Signals::onPipeWarn)>     onPipeWarn;
		SignalWrapper<decltype(Signals::onStateChanged)> onStateChanged;
	};

  public:
	SignalView getSignalView()
	{
		return SignalView{
			{ signals.onEOS },
			{ signals.onPipeError },
			{ signals.onPipeWarn },
			{ signals.onStateChanged },
		};
	}

  private:
	GstElement* fixedPipe;  // Should not changed the pointer after init
	GstElement* fixedSrc;   // Should not changed the pointer after init
	GstElement* fixedSink;  // Should not changed the pointer after init

	Signals  signals;
	Signals& getSignals() { return signals; }

  private:
	std::atomic<TimePoint>    lastPushSuccess = TimePoint::min();
	std::chrono::milliseconds feedTimeout     = std::chrono::milliseconds(50);

  private:
	std::jthread busThread;

  private:
	static void onDecoderPadAdded(GstElement* decoder, GstPad* new_pad, gpointer user_data);

	GstElement* choosePrefDecoder(bool& isDynamic);

  private:
	bool initPipeElements(bool useFileSrc, const char* file_path = nullptr);
	bool initBusThread();

  private:
	GstElement* const pipeline() { return this->fixedPipe; }
	GstElement* const src() { return this->fixedSrc; }
	GstElement* const sink() { return this->fixedSink; }

  public:
	bool tryPushFrame(FramePtr frame);

  public:
	bool play();
	bool pause();
	bool reset();
	bool flush();
	bool stopPipeline();

	StateType getCurrentState();

  public:
	void linkSinkWidget(QQuickItem* widget);

  public:
	/** 
	 * Post a test error to the pipeline, for testing purpose only.
	 * Only works in Debug builds.
	 */
	void postTestError();

  public:
	TVidRender();
	explicit TVidRender(const char* file_path);

	static Ptr create(const char* file_path = nullptr)
	{
		if (file_path) {
			return std::make_shared<TVidRender>(file_path);
		} else {
			return std::make_shared<TVidRender>();
		}
	}

	~TVidRender();

  public:
	TVidRender(const TVidRender&)            = delete;  // Forbid copy or move
	TVidRender& operator=(const TVidRender&) = delete;
	TVidRender(TVidRender&&)                 = delete;
	TVidRender& operator=(TVidRender&&)      = delete;
};
}  // namespace gentau