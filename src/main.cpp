#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Project headers
#include "config.h"
#include "led_controller.h"
#include "sensor_manager.h"
#include "web_handlers.h"
#include "wifi_manager.h"
#include "filesystem_utils.h"

// Global objects
LEDController ledController;
SensorManager sensorManager;
AsyncWebServer server(WEB_SERVER_PORT);
WebHandlers webHandlers(&server, &sensorManager, &ledController);
WiFiManager wifiManager(&ledController);

unsigned long lastSensorDataPrint = 0;
const long printInterval = 200;

void setup()
{
  // Initialize Serial for debugging
  Serial.begin(115200);
  Serial.println("\n\nESP32-S3 NeoPixel Async Web Server Starting...");

  // Initialize LED controller first for status indication
  ledController.init();
  Serial.println("LED Controller initialized");

  // Initialize file system
  if (!FilesystemUtils::initSPIFFS())
  {
    Serial.println("Failed to initialize SPIFFS");
    return;
  }

  // List files and check for index.html
  FilesystemUtils::listFiles();
  FilesystemUtils::checkIndexFile();

  // Initialize WiFi
  if (wifiManager.init())
  {
    // Setup web server routes
    webHandlers.setupRoutes();
    server.begin();
    Serial.println("HTTP async server started");
    Serial.println("Setup completed successfully!");
  }
  else
  {
    Serial.println("WiFi initialization failed!");
  }
}

void loop()
{
  unsigned long currentMilis = millis();

  // Handle WiFi connection and OTA
  wifiManager.handleConnection();

  // Note: AsyncWebServer handles requests automatically, no need to call handleClient()

  if (currentMilis - lastSensorDataPrint >= printInterval)
  {
    Serial.println(sensorManager.getFormattedSensorData(6));
    lastSensorDataPrint = currentMilis;
  }
}