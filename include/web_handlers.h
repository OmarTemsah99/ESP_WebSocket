#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "sensor_manager.h"
#include "led_controller.h"

class WebHandlers
{
private:
    AsyncWebServer *server;
    SensorManager *sensorManager;
    LEDController *ledController;

    String getContentType(String filename);
    void handleStaticFile(AsyncWebServerRequest *request);

public:
    WebHandlers(AsyncWebServer *webServer, SensorManager *sensorMgr, LEDController *ledCtrl);

    void handleRoot(AsyncWebServerRequest *request);
    void handleSensorData(AsyncWebServerRequest *request);
    void handleGetSensorData(AsyncWebServerRequest *request);
    void handleColor(AsyncWebServerRequest *request);
    void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleUpload(AsyncWebServerRequest *request);
    void handleDeleteFile(AsyncWebServerRequest *request);
    void handleListFiles(AsyncWebServerRequest *request);
    void handleFirmware(AsyncWebServerRequest *request);
    void handleFirmwareUpdate(AsyncWebServerRequest *request);
    void handleSensorDataPage(AsyncWebServerRequest *request);
    void setupRoutes();
};

#endif