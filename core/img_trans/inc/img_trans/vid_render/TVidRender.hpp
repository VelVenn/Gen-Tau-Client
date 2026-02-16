#pragma once

#include "utils/TSignal.hpp"
#include "utils/TTypeRedef.hpp"

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
class TVidRender : public std::enable_shared_from_this<TVidRender>
{
  public:
	using FramePtr   = std::unique_ptr<std::vector<u8>>;
	using ElemRawPtr = GstElement*;
	using TimePoint  = std::chrono::steady_clock::time_point;
	using SharedPtr  = std::shared_ptr<TVidRender>;

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

  private:
	GstElement* fixedPipe;  // Should not changed the pointer after init
	GstElement* fixedSrc;   // Should not changed the pointer after init
	GstElement* fixedSink;  // Should not changed the pointer after init

  public:
	TSignal<TVidRender>                                                   onEOS;
	TSignal<TVidRender, IssueType, std::string, std::string, std::string> onPipeError;
	TSignal<TVidRender, IssueType, std::string, std::string, std::string> onPipeWarn;
	TSignal<TVidRender, StateType, StateType>                             onStateChanged;

  private:
	std::atomic<TimePoint> lastPushSuccess = TimePoint::min();
	std::atomic<u64>       maxBufferBytes  = 262'144;  // Default to 256 KB

  public:
	u64  getMaxBufferBytes() const { return maxBufferBytes.load(); }    // MT-SAFE
	void setMaxBufferBytes(u64 bytes) { maxBufferBytes.store(bytes); }  // MT-SAFE

	// MT-SAFE
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
	/** 
	 * @brief: Let the pipeline start playing. 
	 * 
	 * @note: NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		  or the thread that created the TVidRender instance.
	 */
	bool play();

	/**
	 * @brief: Pause the pipeline.
	 * 
	 * @note: NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		  or the thread that created the TVidRender instance.
	 */
	bool pause();

	/**
	 * @brief: Restart the pipeline.
	 * 
	 * @note: NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		  or the thread that created the TVidRender instance. This method 
	 *        will release all the resources to the hardware level. May cause 
	 *        FATAL error on OS X.  
	 */
	bool restart();

	/**
	 * @brief: Flush the pipeline. Will only clear the data in the pipeline. 
	 * 
	 * @note: NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		  or the thread that created the TVidRender instance.
	 */
	bool flush();

	/**
	 * @brief: Stop the pipeline. Will set pipeline to NULL state and release all resources. 
	 * 
	 * @note: NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		  or the thread that created the TVidRender instance. This method 
	 *        will release all the resources to the hardware level. May cause 
	 *        FATAL error on OS X.  
	 */
	bool stop();

	// MT-SAFE
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
	explicit TVidRender(u64 _maxBufferBytes = 262'144);  // Default to 256 KB
	explicit TVidRender(const char* file_path, u64 _maxBufferBytes = 262'144);

	/** 
	 * @brief: create a shared pointer to TVidRender instance. 
	 *
	 * @throws: std::runtime_error if the pipeline initialization failed, or 
	 *          if file_path is provided in non-Debug builds.
	 */
	[[nodiscard("Should not ignored the created TVidRender::SharedPtr")]] static SharedPtr create(
		const char* file_path = nullptr, u64 _maxBufferBytes = 262'144
	)
	{
		if (file_path) {
			return std::make_shared<TVidRender>(file_path, _maxBufferBytes);
		} else {
			return std::make_shared<TVidRender>(_maxBufferBytes);
		}
	}

	/** 
	 * @brief: create a shared pointer to TVidRender instance.
	 *
	 * @throws: std::runtime_error if the pipeline initialization failed.
	 */
	[[nodiscard("Should not ignored the created TVidRender::SharedPtr")]] static SharedPtr create(
		u64 _maxBufferBytes
	)
	{
		return std::make_shared<TVidRender>(_maxBufferBytes);
	}

	static void initContext(int* argc, char** argv[]);

	~TVidRender();

  public:
	TVidRender(const TVidRender&)            = delete;  // Forbid copy or move
	TVidRender& operator=(const TVidRender&) = delete;
	TVidRender(TVidRender&&)                 = delete;
	TVidRender& operator=(TVidRender&&)      = delete;
};
}  // namespace gentau