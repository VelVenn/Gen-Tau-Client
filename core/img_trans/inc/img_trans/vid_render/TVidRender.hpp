#pragma once

#include "utils/TTypeRedef.hpp"

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

extern "C"
{
	struct _GstElement;
	struct _GstPad;
	typedef struct _GstElement GstElement;
	typedef struct _GstPad     GstPad;
	typedef void*              gpointer;
}

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

		template<typename... Args>
		auto disconnect(Args&&... args)
		{
			return signal.disconnect(std::forward<Args>(args)...);
		}

		// Connect to the signal
		template<typename... Args>
		auto operator()(Args&&... args)
		{
			return signal.connect(std::forward<Args>(args)...);
		}

		// Connect to the signal
		template<typename CallableType>
		auto& operator+=(CallableType&& slot)
		{
			signal.connect(std::forward<CallableType>(slot));
			return *this;
		}

		// Disconnect from the signal
		template<typename CallableType>
		auto& operator-=(CallableType&& slot)
		{
			signal.disconnect(std::forward<CallableType>(slot));
			return *this;
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

		~Signals()
		{
			onEOS.disconnect_all();
			onPipeError.disconnect_all();
			onPipeWarn.disconnect_all();
			onStateChanged.disconnect_all();
		}
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
	std::atomic<u64>          maxBufferBytes  = 80000;  // 80 KB
	std::chrono::milliseconds feedTimeout     = std::chrono::milliseconds(50);

  public:
	u64  getMaxBufferBytes() const { return maxBufferBytes.load(); }
	void setMaxBufferBytes(u64 bytes) { maxBufferBytes.store(bytes); }

  public:
	TimePoint getLastPushSuccessTime() const { return lastPushSuccess.load(); }

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
	explicit TVidRender(u64 _maxBufferBytes = 80000);
	explicit TVidRender(const char* file_path, u64 _maxBufferBytes = 80000);

	static Ptr create(const char* file_path = nullptr, u64 _maxBufferBytes = 80000)
	{
		if (file_path) {
			return std::make_shared<TVidRender>(file_path, _maxBufferBytes);
		} else {
			return std::make_shared<TVidRender>(_maxBufferBytes);
		}
	}

	static Ptr create(u64 _maxBufferBytes) { return std::make_shared<TVidRender>(_maxBufferBytes); }

	static void initContext(int* argc, char** argv[]);

	~TVidRender();

  public:
	TVidRender(const TVidRender&)            = delete;  // Forbid copy or move
	TVidRender& operator=(const TVidRender&) = delete;
	TVidRender(TVidRender&&)                 = delete;
	TVidRender& operator=(TVidRender&&)      = delete;
};
}  // namespace gentau