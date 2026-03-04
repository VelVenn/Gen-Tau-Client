#include "utils/TScheduler.hpp"

#include <stop_token>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace gentau {
atomic<TScheduler::TaskHandle> TScheduler::globHndlCount{ 0 };

TScheduler::Task::Task(TaskIdentifier _tid, std::function<void()> _job, NanoSec _inv) :
	tid(_tid),
	job(_job),
	interval(_inv > 0ns ? _inv : 1s)
{}

auto TScheduler::Task::create(TaskIdentifier _tid, std::function<void()> _job, NanoSec _inv)
	-> SharedPtr
{
	return make_shared<Task>(_tid, _job, _inv);
}

bool TScheduler::removeTask(TaskHandle hndl)
{
	scoped_lock lock(mtx);
	auto        iter = tasks.find(hndl);

	if (iter == tasks.end()) { return false; }

	iter->second->cancelled.store(true);
	tasks.erase(iter);
	return true;
}

void TScheduler::run()
{
	if (eventLoop.joinable()) { return; }

	eventLoop = jthread([this](stop_token sToken) {
		while (!sToken.stop_requested()) {
			TaskIdentifier nextTid;
			{
				unique_lock lock(mtx);

				if (taskQueue.empty()) {
					cv.wait(lock, sToken, [this] { return !taskQueue.empty(); });
					continue;
				}

				nextTid  = taskQueue.top();
				auto now = chrono::steady_clock::now();
				if (nextTid.nextRun > now) {
					cv.wait_until(lock, sToken, nextTid.nextRun, [this, nextTid] {
						if (taskQueue.empty()) { return true; }

						return taskQueue.top() < nextTid;
					});
				}

				if (!taskQueue.empty() && taskQueue.top() == nextTid) {
					taskQueue.pop();
				} else {
					continue;
				}
			}

			Task::SharedPtr taskToRun;
			{
				scoped_lock lock(mtx);

				auto iter = tasks.find(nextTid.hndl);
				if (iter == tasks.end()) { continue; }

				taskToRun = iter->second;
			}

			taskToRun->doJob();

			{
				scoped_lock lock(mtx);

				auto iter = tasks.find(nextTid.hndl);
				if (iter != tasks.end()) { taskQueue.push(iter->second->tid); }
			}
		}
	});
}
}  // namespace gentau