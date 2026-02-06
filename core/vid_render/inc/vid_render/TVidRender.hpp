#pragma once

#include "utils/TTypeRedef.hpp"

#include <gst/gst.h>
#include <gst/gstelement.h>

#include "readerwritercircularbuffer.h"

#include "sigslot/signal.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <concepts>
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
	using FramePtr  = std::unique_ptr<std::vector<u8>>;
	using Buffer    = moodycamel::BlockingReaderWriterCircularBuffer<FramePtr>;
	using TimePoint = std::chrono::steady_clock::time_point;
	using Ptr       = std::unique_ptr<TVidRender>;

  public:
	template<SigslotSignal T>
	struct SignalWrapper
	{
		T& signal;

		template<typename... Args>
		auto connect(Args&&... args)
		{
			return signal.connect(std::forward<Args>(args)...);
		}
	};

	struct Signals
	{
		sigslot::signal<>                   onEOS;
		sigslot::signal<u32, std::string>   onError;
		sigslot::signal<u32, std::string>   onWarning;
		sigslot::signal<GstState, GstState> onStateChanged;
	};

	struct SignalView
	{
		SignalWrapper<decltype(Signals::onEOS)>          onEOS;
		SignalWrapper<decltype(Signals::onError)>        onError;
		SignalWrapper<decltype(Signals::onWarning)>      onWarning;
		SignalWrapper<decltype(Signals::onStateChanged)> onStateChanged;
	};

  public:
	SignalView getSignalView()
	{
		return SignalView{
			{ signals.onEOS },
			{ signals.onError },
			{ signals.onWarning },
			{ signals.onStateChanged },
		};
	}

  private:
	GstElement* pipeline;
	GstElement* src;
	GstElement* parser;
	GstElement* bufferQueue;
	GstElement* decoder;
	GstElement* leakyQueue;
	GstElement* colorConv;
	GstElement* uploader;
	GstElement* sinkCapsFilter;
	GstElement* sink;

	Buffer naluBuffer;

	Signals signals;

  private:
	std::atomic<bool>         isRunning         = false;
	std::atomic<TimePoint>    lastFrameFeedTime = std::chrono::steady_clock::now();
	std::chrono::milliseconds feedTimeout       = std::chrono::milliseconds(50);

  private:
	std::jthread feedThread;
	std::jthread busThread;

  private:
	static void onDecoderPadAdded(GstElement* decoder, GstPad* new_pad, gpointer user_data);

	GstElement* choosePrefDecoder(bool& isDynamic);

  private:
	bool initPipeElements(bool useFileSrc, const char* file_path = nullptr);

  public:
	bool tryPushFrame(
		FramePtr frame, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)
	);

  public:
	Signals& getSignals() { return signals; }

  public:
	bool play();
	bool pause();
	bool reset();

  public:
	bool run();
	bool stop();

  public:
	void linkSinkWidget(QQuickItem* widget);
	// bool linkOverlayWidget(u64 idx, QQuickItem* widget);
	// void linkOverlayRoot(QQuickItem* root);

	// TODO: Add sink probe to get rendered frames' timestamp

  public:
	TVidRender();
	TVidRender(const char* file_path);

	~TVidRender();

  public:
	TVidRender(const TVidRender&)            = delete;  // Forbid no copy or move
	TVidRender& operator=(const TVidRender&) = delete;
	TVidRender(TVidRender&&)                 = delete;
	TVidRender& operator=(TVidRender&&)      = delete;
};
}  // namespace gentau