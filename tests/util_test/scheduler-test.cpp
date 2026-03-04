#include "utils/TLog.hpp"
#include "utils/TScheduler.hpp"

#include <atomic>
#include <csignal>
#include <thread>

#define T_LOG_TAG ""

using namespace gentau;

using namespace std::chrono_literals;
using namespace std;

atomic_bool running{ true };

void signalHandler(int signum)
{
	running.store(false);
}

class TestClass
{
  private:
	int count = 0;

  public:
	void foo()
	{
		tLogDebug("Test class counter: {}", count);
		count++;
	}

  public:
	TestClass()  = default;
	~TestClass() = default;
};

int main()
{
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	TScheduler sched;
	sched.run();

	auto task1 = sched.addTask(1s, [] { tLogDebug("Task 1 work at 1Hz"); });
	auto task2 =
		sched.addTask(1.0s / 9, [] { tLogDebug("                      Task 2 work at 9Hz"); });

	TestClass bar;
	auto      taskInClass = sched.addTask(5s, &TestClass::foo, &bar);

	bool removeT1 = false;

	while (running.load()) {
		this_thread::sleep_for(3s);

		if (!removeT1) {
			tLogDebug("Removing task 1...");

			sched.removeTask(task1.value());
			auto task3 = sched.addTask(1.0s / 3, [] { tLogDebug("Task 3 work at 3Hz"); });
			removeT1   = true;
		}
	}

	return 0;
}