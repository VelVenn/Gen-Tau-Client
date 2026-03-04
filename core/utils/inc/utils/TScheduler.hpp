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
	template<typename Rep, typename Period, typename Callable, typename... Args>
		requires std::invocable<Callable, Args...>
	std::optional<TaskHandle> addTask(
		std::chrono::duration<Rep, Period> inv, Callable&& callable, Args&&... args
	);

	bool removeTask(TaskHandle hndl);

	void run();

	void stop()
	{
		if (eventLoop.joinable()) {
			eventLoop.request_stop();
			eventLoop.join();
		}
	}

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
) -> std::optional<TaskHandle> // Template function must be defined in headers.
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