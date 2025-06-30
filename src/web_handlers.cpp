#include "web_handlers.h"
#include <Update.h>

WebHandlers::WebHandlers(WebServer *webServer, SensorManager *sensorMgr, LEDController *ledCtrl)
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

// Stream file in chunks to client
void WebHandlers::sendFileInChunks(File &file, String filename)
{
    const size_t bufSize = 1024;
    uint8_t buf[bufSize];
    size_t totalSent = 0;
    size_t fileSize = file.size();
    server->sendHeader("Content-Length", String(fileSize));
    server->setContentLength(fileSize);
    server->send(200, getContentType(filename), "");

    while (totalSent < fileSize)
    {
        size_t toRead = min(bufSize, fileSize - totalSent);
        size_t bytesRead = file.read(buf, toRead);
        if (bytesRead == 0)
            break;
        server->sendContent((char *)buf, bytesRead);
        totalSent += bytesRead;
    }
    Serial.printf("File sent: %s (%d bytes)\n", filename.c_str(), totalSent);
}

// Serve index.html
void WebHandlers::handleRoot()
{
    File file = SPIFFS.open("/index.html", "r");
    if (!file || file.size() == 0)
    {
        Serial.println("Error: index.html missing or empty");
        server->send(500, "text/plain", "index.html missing or empty");
        return;
    }
    sendFileInChunks(file, "/index.html");
    file.close();
}

// Process sensor POST request
void WebHandlers::handleSensorData()
{
    sensorManager->updateSensorData(
        server->client().remoteIP().toString(),
        server->arg("clientId"),
        server->arg("value").toInt());
    ledController->setSensorIndicator(server->arg("value").toInt());
    server->send(200, "text/plain", "OK");
}

// Return sensor data in JSON
void WebHandlers::handleGetSensorData()
{
    server->send(200, "application/json", sensorManager->getSensorDataJSON());
}

// Control LED color
void WebHandlers::handleColor()
{
    ledController->setColor(
        server->arg("r").toInt(),
        server->arg("g").toInt(),
        server->arg("b").toInt());
    server->send(200, "text/plain", "OK");
}

// Serve file manager UI
void WebHandlers::handleUpload()
{
    File file = SPIFFS.open("/file_manager.html", "r");
    if (!file || file.size() == 0)
    {
        Serial.println("Error: file_manager.html missing or empty");
        server->send(500, "text/plain", "file_manager.html missing or empty");
        return;
    }
    sendFileInChunks(file, "/file_manager.html");
    file.close();
}

// Upload file handler
void WebHandlers::handleFileUpload()
{
    HTTPUpload &upload = server->upload();
    static File fsUploadFile;

    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = "/" + upload.filename;
        if (!filename.endsWith(".html") && !filename.endsWith(".css") && !filename.endsWith(".js") && !filename.endsWith(".bin"))
        {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid file type\"}");
            return;
        }
        fsUploadFile = SPIFFS.open(filename, "w");
    }
    else if (upload.status == UPLOAD_FILE_WRITE && fsUploadFile)
    {
        fsUploadFile.write(upload.buf, upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END && fsUploadFile)
    {
        fsUploadFile.close();
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Upload complete\"}");
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        fsUploadFile.close();
        server->send(500, "application/json", "{\"success\":false,\"message\":\"Upload aborted\"}");
    }
}

// Delete file from SPIFFS
void WebHandlers::handleDeleteFile()
{
    String filename = server->arg("file");
    if (!filename.startsWith("/"))
        filename = "/" + filename;

    if (!SPIFFS.exists(filename))
    {
        server->send(404, "text/plain", "File not found");
        return;
    }
    bool success = SPIFFS.remove(filename);
    server->send(success ? 200 : 500, "text/plain", success ? "Deleted" : "Delete failed");
}

// List files in SPIFFS
void WebHandlers::handleListFiles()
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
    server->send(200, "application/json", json);
}

// Serve firmware update page
void WebHandlers::handleFirmware()
{
    File file = SPIFFS.open("/firmware_update.html", "r");
    if (!file || file.size() == 0)
    {
        server->send(500, "text/plain", "firmware_update.html missing or empty");
        return;
    }
    sendFileInChunks(file, "/firmware_update.html");
    file.close();
}

// Perform firmware update from SPIFFS file
void WebHandlers::handleFirmwareUpdate()
{
    String filename = server->arg("file");
    if (!filename.startsWith("/"))
        filename = "/" + filename;
    if (!filename.endsWith(".bin"))
    {
        server->send(400, "text/plain", "Only .bin files allowed");
        return;
    }
    File firmware = SPIFFS.open(filename, "r");
    if (!firmware)
    {
        server->send(404, "text/plain", "Firmware file not found");
        return;
    }
    if (!Update.begin(firmware.size()))
    {
        server->send(500, "text/plain", "Update begin failed");
        return;
    }
    if (Update.writeStream(firmware) != firmware.size())
    {
        server->send(500, "text/plain", "Write failed");
        return;
    }
    if (!Update.end(true))
    {
        server->send(500, "text/plain", Update.errorString());
        return;
    }
    firmware.close();
    server->send(200, "text/plain", "Update successful. Rebooting...");
    delay(1000);
    ESP.restart();
}

// Serve sensor data page
void WebHandlers::handleSensorDataPage()
{
    File file = SPIFFS.open("/sensor_data.html", "r");
    if (!file || file.size() == 0)
    {
        server->send(500, "text/plain", "sensor_data.html missing or empty");
        return;
    }
    sendFileInChunks(file, "/sensor_data.html");
    file.close();
}

// Serve static files (CSS/JS/HTML fallback)
void WebHandlers::handleStaticFile()
{
    String path = server->uri();
    File file = SPIFFS.open(path, "r");
    if (!file)
    {
        server->send(404, "text/plain", "File not found");
        return;
    }
    sendFileInChunks(file, path);
    file.close();
}

// Define all routes
void WebHandlers::setupRoutes()
{
    server->on("/", HTTP_GET, [this]()
               { handleRoot(); });
    server->on("/sensor", HTTP_POST, [this]()
               { handleSensorData(); });
    server->on("/sensorData", HTTP_GET, [this]()
               { handleGetSensorData(); });
    server->on("/color", HTTP_GET, [this]()
               { handleColor(); });
    server->on("/upload", HTTP_GET, [this]()
               { handleUpload(); });
    server->on("/upload", HTTP_POST, []() {}, [this]()
               { handleFileUpload(); });
    server->on("/delete", HTTP_POST, [this]()
               { handleDeleteFile(); });
    server->on("/list", HTTP_GET, [this]()
               { handleListFiles(); });
    server->on("/firmware", HTTP_GET, [this]()
               { handleFirmware(); });
    server->on("/firmwareUpdate", HTTP_POST, [this]()
               { handleFirmwareUpdate(); });
    server->on("/sensorpage", HTTP_GET, [this]()
               { handleSensorDataPage(); });

    // Fallback static handler
    server->onNotFound([this]()
                       {
        String path = server->uri();
        if (path.endsWith(".html") || path.endsWith(".css") || path.endsWith(".js")) {
            handleStaticFile();
        } else {
            server->send(404, "text/plain", "Not found");
        } });
}
