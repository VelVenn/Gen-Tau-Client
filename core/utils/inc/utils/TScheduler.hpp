#pragma once

#include "utils/TTypeRedef.hpp"

#include <atomic>
#include <chrono>
#include <compare>
#include <concepts>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gentau {
/**
 * TScheduler 是一个轻量级的单线程事件循环调度器，用于管理周期性任务的定时执行。
 *
 * TScheduler 适用于少量、轻量、中低频的周期性任务，如定频 MQTT 消息的异步发布。
 * 它内部维护了独立的线程来运行事件循环，因此可以避免在调用者线程中阻塞。
 *
 * 任务的执行在 TScheduler 的事件循环中串行地进行，因此不适合向 TScheduler
 * 注册耗时较长的任务。如果需要处理大量高频任务或需要并行执行，建议使用多个
 * TScheduler 实例按频率分组调度。
 *
 * TScheduler 会自行管理内部线程的生命周期，因此在绝大多数的情况下，调用者在启动
 * TScheduler 的事件循环后不需要显式地调用 `stop()` 或 `stopAsync()` 来停止
 * 调度器。
 *
 * 调用者需要自行确保被任务引用的外部对象在对应任务的生命周期内都是有效的。
 */
class TScheduler
{
  public:
	using TimePoint  = std::chrono::steady_clock::time_point;
	using NanoSec    = std::chrono::nanoseconds;
	using Sec        = std::chrono::seconds;
	using TaskHandle = u64;

  public:
	struct TaskIdentifier
	{
		TaskHandle hndl;
		TimePoint  nextRun{ TimePoint::min() };

		std::strong_ordering operator<=>(TaskIdentifier other) const
		{
			auto cmp = nextRun <=> other.nextRun;

			if (cmp != 0) { return cmp; }

			return hndl <=> other.hndl;
		}

		bool operator==(TaskIdentifier other) const { return hndl == other.hndl; }
	};

	struct Task
	{
		using SharedPtr = std::shared_ptr<Task>;

		TaskIdentifier        tid;
		std::function<void()> job;
		TimePoint             startTime;
		NanoSec               interval  = NanoSec(Sec(1));
		u64                   execCount = 0;
		std::atomic<bool>     cancelled{ false };

		void doJob()
		{
			if (cancelled.load()) { return; }

			if (execCount == 0) { startTime = std::chrono::steady_clock::now(); }

			if (job) { job(); }

			execCount++;

			tid.nextRun = startTime + execCount * interval;
		}

		void operator()() { doJob(); }

		Task()  = default;
		~Task() = default;

		Task(TaskIdentifier _tid, std::function<void()> _job, NanoSec _inv);
		static SharedPtr create(TaskIdentifier _tid, std::function<void()> _job, NanoSec _inv);

		Task(const Task&)            = delete;
		Task& operator=(const Task&) = delete;
		Task(Task&&)                 = delete;
		Task& operator=(Task&&)      = delete;
	};

	using TaskMap   = std::unordered_map<TaskHandle, Task::SharedPtr>;
	using TaskQueue = std::
		priority_queue<TaskIdentifier, std::vector<TaskIdentifier>, std::greater<TaskIdentifier>>;
	// STL priority_queue is default max-heap (std::less), we use std::greater to create min-heap
	// so that the task with the smallest nextRun is at the top

  private:
	static std::atomic<TaskHandle> globHndlCount;

	TaskMap   tasks;
	TaskQueue taskQueue;

	std::mutex                  mtx;
	std::condition_variable_any cv;

	std::jthread eventLoop;

  public:
	/**
	 * @brief 添加一个周期性任务到调度器中
	 * @param inv 任务执行的周期
	 * @param callable 任务执行的方法
	 * @param args 任务执行的参数
	 * @return 任务的句柄
	 * @note 多线程安全，但是调用者需要自行保证传入的参数的生命周期长于任务的生命周期
	 */
	template<typename Rep, typename Period, typename Callable, typename... Args>
		requires std::invocable<Callable, Args...>
	std::optional<TaskHandle> addTask(
		std::chrono::duration<Rep, Period> inv, Callable&& callable, Args&&... args
	);

	/**
	 * @brief 从调度器中移除一个任务
	 * @param hndl 任务的句柄
	 * @note 多线程安全
	 */
	bool removeTask(TaskHandle hndl);

	/**
	 * @brief 启动调度器
	 * @note 非多线程安全！强烈建议仅在创建了当前 TScheduler 实例的线程中调用此方法。该方法
	 *       可以多次调用，如果检查到调度器已经启动，该方法会立即返回。
	 */
	void run();

	/**
	 * @brief 停止调度器，该方法会阻塞直到调度器完全停止
	 * @note 非多线程安全！强烈建议仅在创建了当前 TScheduler 实例的线程中调用此方法。一般情
	 *       况下，你不需要手动调用此方法。
	 */
	void stop()
	{
		if (eventLoop.joinable()) {
			eventLoop.request_stop();
			eventLoop.join();
		}
	}

	/**
	 * @brief 异步地停止调度器，该方法会立即返回
	 * @note 非多线程安全！强烈建议仅在创建了当前 TScheduler 实例的线程中调用此方法。一般情
	 *       况下，你不需要手动调用此方法。
	 */
	void stopAsync() { eventLoop.request_stop(); }

  public:
	TScheduler()  = default;
	~TScheduler() = default;

	TScheduler(const TScheduler&)            = delete;
	TScheduler& operator=(const TScheduler&) = delete;
	TScheduler(TScheduler&&)                 = delete;
	TScheduler& operator=(TScheduler&&)      = delete;
};

template<typename Rep, typename Period, typename Callable, typename... Args>
	requires std::invocable<Callable, Args...>
auto TScheduler::addTask(
	std::chrono::duration<Rep, Period> inv, Callable&& callable, Args&&... args
) -> std::optional<TaskHandle>
{
	TaskHandle      hndl = globHndlCount.fetch_add(1);
	Task::SharedPtr task = Task::create(
		TaskIdentifier{ hndl },
		[f = std::forward<Callable>(callable), ... args = std::forward<Args>(args)]() mutable {
			std::invoke(f, args...);
		},  // 使用 lambda 闭包防止传入 bind 被自动解包
		std::chrono::duration_cast<NanoSec>(inv)
	);

	// Pack lambda capture: https://en.cppreference.com/w/cpp/language/lambda.html#Lambda_capture

	// Template function must be defined in headers. It won't volatile ODR unless
	// it is fully specialized.

	{
		std::scoped_lock lock(mtx);
		auto [iter, inserted] = tasks.try_emplace(hndl, task);

		if (inserted) {
			taskQueue.push(iter->second->tid);
		} else {
			return std::nullopt;
		}
	}

	cv.notify_one();
	return hndl;
}
}  // namespace gentau