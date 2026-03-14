#pragma once

#include "comm/TMqttClient.hpp"

#include <QObject>
#include <QProtobufSerializer>

class GTCommViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int redHp READ redHp NOTIFY redHpChanged)
    Q_PROPERTY(int blueHp READ blueHp NOTIFY blueHpChanged)
    Q_PROPERTY(int battleTimeSec READ battleTimeSec NOTIFY battleTimeSecChanged)
    Q_PROPERTY(int currentAmmo READ currentAmmo NOTIFY currentAmmoChanged)
    Q_PROPERTY(int currentHp READ currentHp NOTIFY currentHpChanged)
    Q_PROPERTY(int maxHp READ maxHp NOTIFY maxHpChanged)

public:
    explicit GTCommViewModel(QObject* parent = nullptr);
    ~GTCommViewModel() override;

    int redHp() const { return _redHp; }
    int blueHp() const { return _blueHp; }
    int battleTimeSec() const { return _battleTimeSec; }
    int currentAmmo() const { return _currentAmmo; }
    int currentHp() const { return _currentHp; }
    int maxHp() const { return _maxHp; }

Q_SIGNALS:
    void redHpChanged();
    void blueHpChanged();
    void battleTimeSecChanged();
    void currentAmmoChanged();
    void currentHpChanged();    
    void maxHpChanged();

private:
    void setupMqtt();

    QProtobufSerializer _serializer;

    int _redHp = 5000;
    int _blueHp = 5000;
    int _battleTimeSec = 420;
    int _currentAmmo = 100;
    int _currentHp = 1700;
    int _maxHp = 5000;

    gentau::TMqttClient::SharedPtr client;
};