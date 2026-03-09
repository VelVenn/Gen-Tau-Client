#include "comm/TMqttClient.hpp"

#include "mqtt/exception.h"
#include "utils/TLog.hpp"
#include "utils/TLogical.hpp"

#include "mqtt/async_client.h"

#include <memory>
#include <mutex>
#include <stdexcept>

#define T_LOG_TAG_COMM "[MQTT Client] "

using namespace mqtt;
using namespace std;
using namespace std::chrono_literals;

namespace gentau {
class TMqttClient::Callback : public virtual mqtt::callback, public virtual mqtt::iaction_listener
{
	TMqttClient* client;

	// Connection failure
	void on_failure(const token& tok) override
	{
		tCommLogError("Connection attempt failed");
		client->onConnectionFailed(
			tok.get_error_message().empty() ? "Unknown" : tok.get_error_message()
		);
	}

	void on_success(const token& tok) override {}

	// Connection success
	void connected(const string& cause) override
	{
		tCommLogInfo(
			"Connection success, client id: {}, server URL: {}", client->clientId, client->serverURI
		);

		client->subscribeAll();

		client->onConnected();
	}

	void connection_lost(const string& cause) override
	{
		tCommLogError("Connection lost, cause: {}", cause.empty() ? "Unknown" : cause);
		client->onConnectionLost(cause.empty() ? "Unknown" : cause);
	}

	void message_arrived(const_message_ptr msg) override
	{
		if (msg->is_duplicate()) { return; }

		ReceiveHandler handler;
		{
			shared_lock lock(client->topicRegMtx);
			auto        iter = client->topicRegister.find(msg->get_topic());

			if (iter == client->topicRegister.end()) {
				tCommLogDebug("Unsubscribed message recieved, topic: {}", msg->get_topic());
				return;
			}

			handler = iter->second;
		}

		handler(msg->get_payload());
	}

	void delivery_complete(delivery_token_ptr token) override {}

  public:
	using UniPtr = unique_ptr<Callback>;

  public:
	explicit Callback(TMqttClient* client) : client(client) {}

	static UniPtr create(TMqttClient* client) { return make_unique<Callback>(client); }
};

TMqttClient::TMqttClient(const string& _clientId, const string& _serverURI) :
	clientId(_clientId),
	serverURI(_serverURI)
{
	if (anyFalse(clientId, serverURI)) {
		throw invalid_argument("Neither client ID nor server URI can be empty");
	}

	cb = Callback::create(this);

	auto createOpt = create_options_builder()
						 .client_id(clientId)
						 .server_uri(serverURI)
						 .mqtt_version(MQTTVERSION_DEFAULT)
						 .persistence(NO_PERSISTENCE)
						 .persist_qos0(false)
						 .restore_messages(false)
						 .send_while_disconnected(false)
						 .delete_oldest_messages()
						 .finalize();

	cli = make_unique<async_client>(createOpt);
	cli->set_callback(*cb);

	connOpt = make_unique<connect_options>(connect_options_builder()
											   .clean_session()
											   .automatic_reconnect(1s, 10s)
											   .connect_timeout(5s)
											   .keep_alive_interval(5s)
											   .max_inflight(25)
											   .finalize());
}

TMqttClient::~TMqttClient() = default;

void TMqttClient::subscribeAll()
{
	shared_lock lock(topicRegMtx);

	for (auto& topicReg : topicRegister) {
		cli->subscribe(topicReg.first, static_cast<i32>(QoS::AT_LEAST_ONCE));
	}
}

void TMqttClient::publish(const std::string& topic, const std::string& payload, QoS qos)
{
	cli->publish(topic, payload, static_cast<i32>(qos), false);
}

void TMqttClient::subscribe(const std::string& topic, ReceiveHandler handler)
{
	{
		unique_lock lock(topicRegMtx);
		auto [iter, inserted] = topicRegister.try_emplace(topic, handler);

		if (!inserted) {
			tCommLogWarn("Topic already subscribed: {}, ignoring this", topic);

			return;
		}
	}

	if (cli->is_connected()) { cli->subscribe(topic, static_cast<i32>(QoS::AT_LEAST_ONCE)); }
}

void TMqttClient::connect()
{
	if (cli->is_connected()) {
		tCommLogWarn("Already connected, ignoring this");
		return;
	}

	cli->connect(*connOpt, nullptr, *cb);
}

void TMqttClient::disconnect()
{
	tCommLogInfo("Trying to disconnect client...");

	try {
		// A value of zero or less means the client will not quiesce.
		cli->disconnect(0)->wait();
		tCommLogInfo("Client disconnected successfully");
	} catch (const mqtt::exception& exc) {
		tCommLogError("Client disconnect failed: {}", exc.what());
	}
}

void TMqttClient::rebind(const string& _clientId, const string& _serverURI)
{
	if (anyFalse(_clientId, _serverURI)) {
		throw invalid_argument("Neither client ID nor server URI can be empty");
	}

	if (this->clientId == _clientId && this->serverURI == _serverURI) { return; }

	tCommLogInfo(
		"Atempting to rebind client id: {} -> {}, server: {} -> {}",
		clientId,
		_clientId,
		serverURI,
		_serverURI
	);

	disconnect();

	clientId  = _clientId;
	serverURI = _serverURI;

	auto createOpt = create_options_builder()
						 .client_id(clientId)
						 .server_uri(serverURI)
						 .mqtt_version(MQTTVERSION_DEFAULT)
						 .persistence(NO_PERSISTENCE)
						 .persist_qos0(false)
						 .restore_messages(false)
						 .send_while_disconnected(false)
						 .delete_oldest_messages()
						 .finalize();

	cli = make_unique<async_client>(createOpt);
	cli->set_callback(*cb);

	cli->connect(*connOpt, nullptr, *cb);
}
}  // namespace gentau