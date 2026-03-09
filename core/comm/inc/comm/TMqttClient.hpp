#pragma once

#include "utils/TSignal.hpp"
#include "utils/TTypeRedef.hpp"

#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace mqtt {
class async_client;
class connect_options;
}  // namespace mqtt

namespace gentau {
class TMqttClient
{
  public:
	using SharedPtr        = std::shared_ptr<TMqttClient>;
	using ReceiveHandler   = std::function<void(const std::string& /*Payload*/)>;
	using HandlerSignal    = TSignal<TMqttClient, const std::string&>;
	using HandlerSignalPtr = std::shared_ptr<HandlerSignal>;
	using TopicRegister    = std::unordered_map<std::string, HandlerSignalPtr>;

  public:
	enum class QoS : i32
	{
		AT_MOST_ONCE  = 0,
		AT_LEAST_ONCE = 1,
		EXACTLY_ONCE  = 2
	};

  private:
	class Callback;

  private:
	std::unique_ptr<mqtt::async_client>    cli;
	std::unique_ptr<Callback>              cb;
	std::unique_ptr<mqtt::connect_options> connOpt;

	TopicRegister topicRegister;

	std::string clientId;
	std::string serverURI;

	std::shared_mutex topicRegMtx;

  public:
	TSignal<TMqttClient>                        onConnected;
	TSignal<TMqttClient, std::string /*Cause*/> onConnectionLost;    // `std::string` (cause)
	TSignal<TMqttClient, std::string /*Cause*/> onConnectionFailed;  // `std::string` (cause)

  private:
	void subscribeAll();

  public:
	void publish(
		const std::string& topic, const std::string& payload, QoS qos = QoS::AT_LEAST_ONCE
	);
	Connection subscribe(const std::string& topic, ReceiveHandler handler);

	void connect();
	void disconnect();
	void rebind(
		const std::string& _clientId, const std::string& _serverURI = "mqtt://localhost:3333"
	);

  public:
	explicit TMqttClient(
		const std::string& _clientId, const std::string& _serverURI = "mqtt://localhost:3333"
	);

	static SharedPtr create(
		const std::string& _clientId, const std::string& _serverURI = "mqtt://localhost:3333"
	)
	{
		return std::make_shared<TMqttClient>(_clientId, _serverURI);
	}

	~TMqttClient();

	TMqttClient(const TMqttClient&)            = delete;
	TMqttClient& operator=(const TMqttClient&) = delete;
	TMqttClient(TMqttClient&&)                 = delete;
	TMqttClient& operator=(TMqttClient&&)      = delete;
};
}  // namespace gentau
