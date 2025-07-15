#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <map>
#include <string>
#include <Arduino.h>

struct SensorData
{
    String clientId;
    int touchValue;
    float batteryVoltage;
    float batteryPercent;
};

class SensorManager
{
private:
    std::map<String, SensorData> sensorDataMap;

public:
    void updateSensorData(const String &senderIP, const String &clientId, int touchValue, float batteryVoltage, float batteryPercent);
    String getSensorDataJSON() const;
    const std::map<String, SensorData> &getAllSensorData() const;
    void clearSensorData();
    bool hasSensorData() const;
    String getFormattedSensorData() const;               // Uses touchValue
    String getFormattedSensorData(int minSensors) const; // Uses touchValue
};

#endif // SENSOR_MANAGER_H