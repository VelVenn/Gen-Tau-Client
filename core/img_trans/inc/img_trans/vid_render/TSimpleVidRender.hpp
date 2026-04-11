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
class TSimpleVidRender : public std::enable_shared_from_this<TSimpleVidRender>
{
  private:
	using FramePtr   = std::unique_ptr<std::vector<u8>>;
	using ElemRawPtr = GstElement*;

  public:
	using TimePoint = std::chrono::steady_clock::time_point;
	using SharedPtr = std::shared_ptr<TSimpleVidRender>;

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

		GENERIC  // TSimpleVidRender self detected issue, not directly from GStreamer
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
	TSignal<TSimpleVidRender> onEOS;  // End of stream detected

	TSignal<
		TSimpleVidRender,
		IssueType,
		std::string /*Err src*/,
		std::string /*Err msg*/,
		std::string /*Debug info*/>
		onPipeError;  // `gentau::TSimpleVidRender::IssueType`, `std::string` (src), `std::string` (msg), `std::string` (debug info)

	TSignal<
		TSimpleVidRender,
		IssueType,
		std::string /*Warn src*/,
		std::string /*Warn msg*/,
		std::string /*Debug info*/>
		onPipeWarn;  // `gentau::TSimpleVidRender::IssueType`, `std::string` (src), `std::string` (msg), `std::string` (debug info)

	TSignal<TSimpleVidRender, StateType /*Old state*/, StateType /*New state*/>
		onStateChanged;  // `gentau::TSimpleVidRender::StateType` (old state), `gentau::TSimpleVidRender::StateType` (new state)

  private:
	std::atomic<TimePoint> lastPushSuccess = TimePoint::min();
	std::atomic<u64>       maxBufferBytes  = 262'144;  // Default to 256 KB

  private:
	std::jthread busThread;

  public:
	// MT-SAFE
	u64 getMaxBufferBytes() const { return maxBufferBytes.load(); }

	// MT-SAFE, but should be used with caution as it may cause pushFrame to fail if set too low.
	void setMaxBufferBytes(u64 bytes) { maxBufferBytes.store(bytes); }

	// MT-SAFE
	TimePoint getLastPushSuccessTime() const { return lastPushSuccess.load(); }

  private:
	static void onDecoderPadAdded(GstElement* decoder, GstPad* new_pad, gpointer user_data);

	GstElement* choosePrefDecoder(bool& isDynamic);

  private:
	bool initPipeElements();
	bool initBusThread();

  private:
	GstElement* pipeline() { return this->fixedPipe; }
	GstElement* src() { return this->fixedSrc; }
	GstElement* sink() { return this->fixedSink; }

  public:
	/** 
	 * @brief Let the pipeline start playing. 
	 * 
	 * @note NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		  or the thread that created the TSimpleVidRender instance.
	 */
	bool play();

	/**
	 * @brief Pause the pipeline.
	 * 
	 * @note NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		 or the thread that created the TSimpleVidRender instance.
	 */
	bool pause();

	/**
	 * @brief Restart the pipeline.
	 * 
	 * @note NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		 or the thread that created the TSimpleVidRender instance. This method 
	 *       will release all the resources to the hardware level. May cause 
	 *       FATAL error on OS X.  
	 */
	bool restart();

	/**
	 * @brief Flush the pipeline. Will only clear the data in the pipeline. 
	 * 
	 * @note NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		 or the thread that created the TSimpleVidRender instance.
	 */
	bool flush();

	/**
	 * @brief Stop the pipeline. Will set pipeline to NULL state and release all resources. 
	 * 
	 * @note NOT MT-SAFE! Strongly recommend to call this from the main thread 
	 * 		 or the thread that created the TSimpleVidRender instance. This method 
	 *       will release all the resources to the hardware level. May cause 
	 *       FATAL error on OS X.  
	 */
	bool stop();

	// MT-SAFE
	StateType getCurrentState();

  public:
	/**
	 * @brief Link the video output sink to a QQuickItem. This method MUST be called before
	 *        the pipeline is set to playing state.
	 * @note NOT MT-SAFE!
	 */
	void linkSinkWidget(QQuickItem* widget);

  public:
	explicit TSimpleVidRender(
		u64 _maxBufferBytes = 262'144
	);  // Default to 256 KB

	/** 
	 * @brief create a shared pointer to TSimpleVidRender instance.
	 *
	 * @throws std::runtime_error if the pipeline initialization failed.
	 */
	[[nodiscard("Should not ignored the created TSimpleVidRender::SharedPtr")]] static SharedPtr
	create(u64 _maxBufferBytes = 262'144)
	{
		return std::make_shared<TSimpleVidRender>(_maxBufferBytes);
	}

	/**
	 * @brief Initialize the GStreamer context. Must be called before creating any TSimpleVidRender instance.
	 * @param argc Pointer to the argc parameter from the main function.
	 * @param argv Pointer to the argv parameter from the main function.
	 */
	static void initContext(int* argc, char** argv[]);

	~TSimpleVidRender();

  public:
	TSimpleVidRender(const TSimpleVidRender&)            = delete;  // Forbid copy or move
	TSimpleVidRender& operator=(const TSimpleVidRender&) = delete;
	TSimpleVidRender(TSimpleVidRender&&)                 = delete;
	TSimpleVidRender& operator=(TSimpleVidRender&&)      = delete;
};
}  // namespace gentau