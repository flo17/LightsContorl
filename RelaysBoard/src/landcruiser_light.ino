#include <WiFiClient.h>
#include "credentials.h"
#include <ElegantOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include "light.hpp"
#include "mqtt.hpp"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup()
{
  Serial.begin(115200);

  init_pins();

  WiFi.onEvent(WiFiEvent);
  AsyncMqttClient *mqttClient = InitMqtt();
  ConnectWiFi_STA();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String html = "<html><body><h1>Restart ESP8266</h1>";
    html += "<button onclick=\"location.href='/restart'\">Restart</button>";
    html += "</body></html>";
    request->send(200, "text/html", html); });

  // Redirection management for the “Restart” button
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", "Restart in progress...");
    delay(500);  // Wait a while before restarting to send the response
    ESP.restart(); });

  ElegantOTA.begin(&server); // Start ElegantOTA
  WebSerial.begin(&server);
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  ElegantOTA.loop();
  updateEffect();
}
