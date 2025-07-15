#include "web_handlers.h"
#include <Update.h>

WebHandlers::WebHandlers(AsyncWebServer *webServer, SensorManager *sensorMgr, LEDController *ledCtrl)
    : server(webServer), sensorManager(sensorMgr), ledController(ledCtrl) {}

// Determine MIME type based on file extension
String WebHandlers::getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    if (filename.endsWith(".css"))
        return "text/css";
    if (filename.endsWith(".js"))
        return "application/javascript";
    if (filename.endsWith(".json"))
        return "application/json";
    return "text/plain";
}

// Serve index.html
void WebHandlers::handleRoot(AsyncWebServerRequest *request)
{
    if (SPIFFS.exists("/index.html"))
    {
        request->send(SPIFFS, "/index.html", "text/html");
    }
    else
    {
        Serial.println("Error: index.html missing");
        request->send(500, "text/plain", "index.html missing or empty");
    }
}

// Process sensor POST request
void WebHandlers::handleSensorData(AsyncWebServerRequest *request)
{
    int touchValue = 0;
    float batteryVoltage = 0.0;
    float batteryPercent = 0.0;
    String clientId = "";

    if (request->hasParam("touch", true))
        touchValue = request->getParam("touch", true)->value().toInt();
    if (request->hasParam("batteryVoltage", true))
        batteryVoltage = request->getParam("batteryVoltage", true)->value().toFloat();
    if (request->hasParam("batteryPercent", true))
        batteryPercent = request->getParam("batteryPercent", true)->value().toFloat();
    if (request->hasParam("clientId", true))
        clientId = request->getParam("clientId", true)->value();

    sensorManager->updateSensorData(
        request->client()->remoteIP().toString(),
        clientId,
        touchValue,
        batteryVoltage,
        batteryPercent);

    ledController->setSensorIndicator(touchValue);
    request->send(200, "text/plain", "OK");
}

// Return sensor data in JSON
void WebHandlers::handleGetSensorData(AsyncWebServerRequest *request)
{
    request->send(200, "application/json", sensorManager->getSensorDataJSON());
}

// Control LED color
void WebHandlers::handleColor(AsyncWebServerRequest *request)
{
    int r = 0, g = 0, b = 0;

    if (request->hasParam("r"))
        r = request->getParam("r")->value().toInt();
    if (request->hasParam("g"))
        g = request->getParam("g")->value().toInt();
    if (request->hasParam("b"))
        b = request->getParam("b")->value().toInt();

    ledController->setColor(r, g, b);
    request->send(200, "text/plain", "OK");
}

// Serve file manager UI
void WebHandlers::handleUpload(AsyncWebServerRequest *request)
{
    if (SPIFFS.exists("/file_manager.html"))
    {
        request->send(SPIFFS, "/file_manager.html", "text/html");
    }
    else
    {
        Serial.println("Error: file_manager.html missing");
        request->send(500, "text/plain", "file_manager.html missing or empty");
    }
}

// Upload file handler
void WebHandlers::handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    static File uploadFile;

    if (!index)
    {
        Serial.printf("UploadStart: %s\n", filename.c_str());

        // Check file extension
        if (!filename.endsWith(".html") && !filename.endsWith(".css") &&
            !filename.endsWith(".js") && !filename.endsWith(".bin"))
        {
            Serial.println("Invalid file type");
            return;
        }

        String filepath = "/" + filename;
        uploadFile = SPIFFS.open(filepath, "w");
        if (!uploadFile)
        {
            Serial.println("Failed to open file for writing");
            return;
        }
    }

    if (uploadFile)
    {
        uploadFile.write(data, len);
    }

    if (final)
    {
        if (uploadFile)
        {
            uploadFile.close();
            Serial.printf("UploadEnd: %s, %u bytes\n", filename.c_str(), index + len);
        }
    }
}

// Delete file from SPIFFS
void WebHandlers::handleDeleteFile(AsyncWebServerRequest *request)
{
    String filename = "";
    if (request->hasParam("file", true))
        filename = request->getParam("file", true)->value();

    if (!filename.startsWith("/"))
        filename = "/" + filename;

    if (!SPIFFS.exists(filename))
    {
        request->send(404, "text/plain", "File not found");
        return;
    }

    bool success = SPIFFS.remove(filename);
    request->send(success ? 200 : 500, "text/plain", success ? "Deleted" : "Delete failed");
}

// List files in SPIFFS
void WebHandlers::handleListFiles(AsyncWebServerRequest *request)
{
    String json = "[";
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    bool first = true;

    while (file)
    {
        if (!first)
            json += ",";
        json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
        first = false;
        file = root.openNextFile();
    }
    json += "]";
    request->send(200, "application/json", json);
}

// Serve firmware update page
void WebHandlers::handleFirmware(AsyncWebServerRequest *request)
{
    if (SPIFFS.exists("/firmware_update.html"))
    {
        request->send(SPIFFS, "/firmware_update.html", "text/html");
    }
    else
    {
        request->send(500, "text/plain", "firmware_update.html missing or empty");
    }
}

// Perform firmware update from SPIFFS file
void WebHandlers::handleFirmwareUpdate(AsyncWebServerRequest *request)
{
    String filename = "";
    if (request->hasParam("file", true))
        filename = request->getParam("file", true)->value();

    if (!filename.startsWith("/"))
        filename = "/" + filename;
    if (!filename.endsWith(".bin"))
    {
        request->send(400, "text/plain", "Only .bin files allowed");
        return;
    }

    File firmware = SPIFFS.open(filename, "r");
    if (!firmware)
    {
        request->send(404, "text/plain", "Firmware file not found");
        return;
    }

    if (!Update.begin(firmware.size()))
    {
        request->send(500, "text/plain", "Update begin failed");
        firmware.close();
        return;
    }

    if (Update.writeStream(firmware) != firmware.size())
    {
        request->send(500, "text/plain", "Write failed");
        firmware.close();
        return;
    }

    if (!Update.end(true))
    {
        request->send(500, "text/plain", Update.errorString());
        firmware.close();
        return;
    }

    firmware.close();
    request->send(200, "text/plain", "Update successful. Rebooting...");
    delay(1000);
    ESP.restart();
}

// Serve sensor data page
void WebHandlers::handleSensorDataPage(AsyncWebServerRequest *request)
{
    if (SPIFFS.exists("/sensor_data.html"))
    {
        request->send(SPIFFS, "/sensor_data.html", "text/html");
    }
    else
    {
        request->send(500, "text/plain", "sensor_data.html missing or empty");
    }
}

// Serve static files (CSS/JS/HTML fallback)
void WebHandlers::handleStaticFile(AsyncWebServerRequest *request)
{
    String path = request->url();
    if (SPIFFS.exists(path))
    {
        request->send(SPIFFS, path, getContentType(path));
    }
    else
    {
        request->send(404, "text/plain", "File not found");
    }
}

// Define all routes
void WebHandlers::setupRoutes()
{
    // Root route
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleRoot(request); });

    // Sensor routes
    server->on("/sensor", HTTP_POST, [this](AsyncWebServerRequest *request)
               { handleSensorData(request); });

    server->on("/sensorData", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleGetSensorData(request); });

    server->on("/sensorpage", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleSensorDataPage(request); });

    // LED control
    server->on("/color", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleColor(request); });

    // File management routes
    server->on("/upload", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleUpload(request); });

    server->on("/upload", HTTP_POST, [](AsyncWebServerRequest *request)
               { request->send(200, "application/json", "{\"success\":true,\"message\":\"Upload complete\"}"); }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
               { handleFileUpload(request, filename, index, data, len, final); });

    server->on("/delete", HTTP_POST, [this](AsyncWebServerRequest *request)
               { handleDeleteFile(request); });

    server->on("/list", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleListFiles(request); });

    // Firmware update routes
    server->on("/firmware", HTTP_GET, [this](AsyncWebServerRequest *request)
               { handleFirmware(request); });

    server->on("/firmwareUpdate", HTTP_POST, [this](AsyncWebServerRequest *request)
               { handleFirmwareUpdate(request); });

    // Static file handler for CSS, JS, HTML files
    server->onNotFound([this](AsyncWebServerRequest *request)
                       {
        String path = request->url();
        if (path.endsWith(".html") || path.endsWith(".css") || path.endsWith(".js")) {
            handleStaticFile(request);
        } else {
            request->send(404, "text/plain", "Not found");
        } });
}