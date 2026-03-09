#include "comm/TMqttClient.hpp"
#include "utils/TLog.hpp"
#include "utils/TScheduler.hpp"

#include "proto/test.pb.h"

#include <atomic>
#include <csignal>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#define T_LOG_TAG "[MQTT Test Pub] "

using namespace gentau;
using namespace std;
using namespace std::chrono_literals;

using QoS       = TMqttClient::QoS;
using HandlePtr = shared_ptr<optional<u64>>;

atomic_bool isRunning{ true };

void onSignal(int)
{
	tLogInfo("Signal received, shutting down...");
	isRunning.store(false);
}

int main()
{
	std::signal(SIGINT, onSignal);
	std::signal(SIGTERM, onSignal);

	auto client = TMqttClient::create("gen-tau-test-pub");

	auto sched = TScheduler::create();

	HandlePtr task1 = make_shared<optional<u64>>();

	client->onConnected += [cliWeakRef = TMqttClient::weakRef(client), sched, task1] {
		if (task1 == nullptr) { return; }

		*task1 = sched->addTask(1s, [cliWeakRef]() {
			static u32 counter = 0;

			if (auto cli = cliWeakRef.lock()) {
				test_proto::RobotPerformanceSelectionCommand pubMsg;

				pubMsg.set_shooter((counter % 4) + 1);
				pubMsg.set_chassis(((counter + 1) % 4) + 1);
				pubMsg.set_sentry_control(counter % 2);

				string payload;
				if (pubMsg.SerializeToString(&payload)) {
					cli->publish("RobotPerformanceSelectionCommand", payload, QoS::AT_LEAST_ONCE);
					tLogDebug("RobotPerformanceSelectionCommand published: {}", counter);
				} else {
					tLogError("Failed to serialize RobotPerformanceSelectionCommand message!");
				}

				counter++;
			}
		});
	};

	client->onConnectionLost +=
		[cliWeakRef = TMqttClient::weakRef(client), sched, task1](const string& cause) {
			tLogError("Connection lost: {}", cause);
			if (task1 && (task1->has_value())) {
				sched->removeTask(task1->value());
				*task1 = nullopt;
			}
		};

	client->onConnectionFailed += [](const string& cause) {
		tLogError("Connection failed: {}", cause);
	};

	sched->run();

	client->connect();

	while (isRunning.load()) { this_thread::sleep_for(1s); }

	return 0;
}