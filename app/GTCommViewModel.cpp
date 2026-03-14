#include "GTCommViewModel.hpp"
#include <QDebug>
#include <QMetaObject>
#include <stdexcept>
#include "comm/TMqttClient.hpp"
#include "topics.qpb.h"

#include <string>

using namespace gentau;
using namespace std;

GTCommViewModel::GTCommViewModel(QObject* parent) :
	QObject(parent),
	client(TMqttClient::create("101"))
{
	if (!client) { throw invalid_argument("Client can't be nullptr"); }

	setupMqtt();
}

GTCommViewModel::~GTCommViewModel() {}

void GTCommViewModel::setupMqtt()
{
	// Important: MQTT connection error handling is omitted for brevity, but TMqttClient connects in the background.
	client->connect();

	client->registerTopic("GameStatus", [this](const string& payload) {
		Gentau::Topics::GameStatus msg;

		msg.deserialize(&_serializer, QByteArrayView(payload.data(), payload.size()));

		if (_serializer.lastError() != QAbstractProtobufSerializer::Error::None) {
			qWarning() << "Failed to deserialize GameStatus: " << _serializer.lastErrorString();
			return;
		}

		QMetaObject::invokeMethod(this->parent(), [this, time = msg.stageCountdownSec()]() {
			_battleTimeSec = time;
			Q_EMIT battleTimeSecChanged();
		});
	});

	client->registerTopic("GlobalUnitStatus", [this](const string& payload) {
		Gentau::Topics::GlobalUnitStatus msg;

		msg.deserialize(&_serializer, QByteArrayView(payload.data(), payload.size()));

		if (_serializer.lastError() != QAbstractProtobufSerializer::Error::None) {
			qWarning() << "Failed to deserialize GlobalUnitStatus: "
					   << _serializer.lastErrorString();
			return;
		}

		QMetaObject::invokeMethod(
			this->parent(), [this, red = msg.baseHealth(), blue = msg.enemyBaseHealth()]() {
				_redHp  = red;
				_blueHp = blue;
				Q_EMIT redHpChanged();
				Q_EMIT blueHpChanged();
			}
		);
	});

	client->registerTopic("RobotDynamicStatus", [this](const string& payload) {
		Gentau::Topics::RobotDynamicStatus msg;

		msg.deserialize(&_serializer, QByteArrayView(payload.data(), payload.size()));

		if (_serializer.lastError() != QAbstractProtobufSerializer::Error::None) {
			qWarning() << "Failed to deserialize RobotDynamicStatus: "
					   << _serializer.lastErrorString();
			return;
		}

		QMetaObject::invokeMethod(
			this->parent(), [this, hp = msg.currentHealth(), ammo = msg.remainingAmmo()]() {
				_currentHp   = hp;
				_currentAmmo = ammo;
				Q_EMIT currentHpChanged();
				Q_EMIT currentAmmoChanged();
			}
		);
	});
}
