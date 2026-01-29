#pragma once

#include "utils/TTypeRedef.hpp"

#include <gst/gst.h>
#include <gst/gstelement.h>

#include "readerwritercircularbuffer.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <ratio>
#include <thread>
#include <vector>

class QQuickItem;

namespace gentau {
class TVidRender
{
  public:
	using FramePtr = std::unique_ptr<std::vector<u8>>;
	using Buffer   = moodycamel::BlockingReaderWriterCircularBuffer<FramePtr>;
	using Ptr      = std::unique_ptr<TVidRender>;

  private:
	GstElement* pipeline;
	GstElement* src;
	GstElement* parser;
	GstElement* decoder;
	GstElement* vconv;
	GstElement* conv;
	GstElement* sink;

	Buffer frameBuffer;

  private:
	std::atomic<bool> isRunning;
	std::jthread      feedThread;
	std::jthread      busThread;

  private:
	void onDecoderPadAdded();

  public:
	bool tryPushFrame(
		FramePtr frame, std::chrono::duration<u32, std::nano> timeout = std::chrono::nanoseconds(0)
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
	TVidRender(const TVidRender&)            = delete; // Forbid no copy or move
	TVidRender& operator=(const TVidRender&) = delete;
    TVidRender(TVidRender&&)                 = delete;
    TVidRender& operator=(TVidRender&&)      = delete;
};
}  // namespace gentau