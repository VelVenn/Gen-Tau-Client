#include "comm/TMqttClient.hpp"
#include "utils/TLog.hpp"
#include "utils/TScheduler.hpp"

#include "test_proto/test.pb.h"

#include <atomic>
#include <csignal>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

#define T_LOG_TAG "[MQTT Test] "

using namespace gentau;
using namespace std;
using namespace std::chrono_literals;

atomic_bool isRunning{ true };

void onSignal(int sig)
{
	isRunning.store(false);
}

int main()
{
	std::signal(SIGINT, onSignal);
	std::signal(SIGTERM, onSignal);

	auto client = TMqttClient::create("101");

    client->onConnectionFailed += [](const string& cause) {
        tLogCritical("Failed to connect the broker: {}", cause);

        isRunning.store(false);
    };

	client->connect();

	client->subscribe("Testing", [](const string& payload) {
		std::cout << "\n=============================================" << std::endl;

		// 反序列化过程
		test_proto::Testing testing_msg;

		if (testing_msg.ParseFromString(payload)) {
			std::cout << "[Proto] Successfully deserialized 'Testing' message:" << std::endl;
			// 打印各个字段
			std::cout << "  - test (uint32): " << testing_msg.test() << std::endl;
			std::cout << "  - test_bi (bool): " << (testing_msg.test_bi() ? "true" : "false")
					  << std::endl;
			std::cout << "  - test_int (int32): " << testing_msg.test_int() << std::endl;
		} else {
			std::cerr << "[Error] Failed to parse Protobuf message from payload!" << std::endl;
		}
		std::cout << "=============================================\n" << std::endl;
	});

    while (isRunning.load()) {
        this_thread::sleep_for(1s);
    }

	return 0;
}