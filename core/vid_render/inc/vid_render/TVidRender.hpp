#pragma once

#include "utils/TTypeRedef.hpp"

#include <gst/gst.h>

#include "readerwritercircularbuffer.h"

#include "sigslot/signal.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

class QQuickItem;

namespace gentau {
class TVidRender
{
  public:
	using FramePtr  = std::unique_ptr<std::vector<u8>>;
	using Buffer    = moodycamel::BlockingReaderWriterCircularBuffer<FramePtr>;
	using TimePoint = std::chrono::steady_clock::time_point;
	using Ptr       = std::unique_ptr<TVidRender>;

  private:
	GstElement* pipeline;
	GstElement* src;
	GstElement* parser;
	GstElement* queue;
	GstElement* decoder;
	GstElement* vconv;
	GstElement* conv;
	GstElement* sink;

	Buffer naluBuffer;

  private:
	std::atomic<bool>         isRunning         = false;
	std::atomic<TimePoint>    lastFrameFeedTime = std::chrono::steady_clock::now();
	std::chrono::milliseconds feedTimeout       = std::chrono::milliseconds(50);

  private:
	std::jthread feedThread;
	std::jthread busThread;

  private:
	static void onDecoderPadAdded(GstElement* decoder, GstPad* new_pad, gpointer user_data);

  private:
	bool initPipeElements();

  public:
	bool tryPushFrame(
		FramePtr frame, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)
	);

  public:
	bool play();
	bool pause();
	bool reset();

  public:
	bool run();
	bool stop();

  public:
	void linkWidget(QQuickItem* widget);

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