#include "sensor_manager.h"
#include <vector>
#include <algorithm>

void SensorManager::updateSensorData(const String &senderIP, const String &clientId, int touchValue, float batteryVoltage, float batteryPercent)
{
    sensorDataMap[senderIP] = {clientId, touchValue, batteryVoltage, batteryPercent};
    // Serial.printf("Sensor update from %s: clientId=%s, value=%d\n", senderIP.c_str(), clientId.c_str(), sensorValue);
}

String SensorManager::getSensorDataJSON() const
{
    String json = "{";
    bool first = true;
    for (const auto &pair : sensorDataMap)
    {
        if (!first)
            json += ",";
        json += "\"" + pair.first + "\":{";
        json += "\"clientId\":\"" + pair.second.clientId + "\",";
        json += "\"touch\":" + String(pair.second.touchValue) + ",";
        json += "\"batteryVoltage\":" + String(pair.second.batteryVoltage, 2) + ",";
        json += "\"batteryPercent\":" + String(pair.second.batteryPercent, 1);
        json += "}";
        first = false;
    }
    json += "}";
    return json;
}

const std::map<String, SensorData> &SensorManager::getAllSensorData() const
{
    return sensorDataMap;
}

void SensorManager::clearSensorData()
{
    sensorDataMap.clear();
}

bool SensorManager::hasSensorData() const
{
    return !sensorDataMap.empty();
}

String SensorManager::getFormattedSensorData() const
{
    return getFormattedSensorData(0); // Call the parameterized version
}

String SensorManager::getFormattedSensorData(int minSensors) const
{
    // Find the maximum client ID
    int maxId = -1;
    for (const auto &pair : sensorDataMap)
    {
        int id = pair.second.clientId.toInt();
        if (id > maxId)
            maxId = id;
    }

    // Determine the actual number of slots needed
    int numSlots = (minSensors > (maxId + 1)) ? minSensors : (maxId + 1);
    if (numSlots == 0)
        return "TP:"; // Handle no sensors case

    // Create a vector to hold values in their correct positions
    std::vector<int> values(numSlots, 0);

    // Fill the vector with sensor values at their correct positions
    for (const auto &pair : sensorDataMap)
    {
        int id = pair.second.clientId.toInt();
        if (id >= 0 && id < numSlots)
        {
            values[id] = pair.second.touchValue;
        }
    }

    // Build the result string
    String result = "TP:";
    for (int i = 0; i < numSlots; i++)
    {
        if (i > 0)
            result += ",";
        result += String(values[i]);
    }

    return result;
}