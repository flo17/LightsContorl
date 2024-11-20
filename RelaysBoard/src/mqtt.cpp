#include "mqtt.hpp"
#include "credentials.h"
#include "effects.h"
#include <Ticker.h>
#include <WebSerial.h>
#include "light.hpp"
#include <ArduinoJson.h>

// Defining WiFi channel for optimized connection speed
#define WIFI_CHANNEL 6

Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;

AsyncMqttClient mqttClient;

void ConnectWiFi_STA()
{
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
    String message = "Connecting to WiFi..."; // Create message with IP

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void ConnectToMqtt()
{
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    String ipStr = WiFi.localIP().toString();   // Get IP address as a string
    String message = "Connected! IP: " + ipStr; // Create message with IP
    switch (event)
    {
    case WIFI_EVENT_STAMODE_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        // Connection successful, retrieve IP address

        ConnectToMqtt();
        break;
    case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
        Serial.println("WiFi lost connection");
        mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        wifiReconnectTimer.once(2, ConnectWiFi_STA);
        break;
    }
}

void SuscribeMqtt()
{
    mqttClient.subscribe(TOPIC_LIGHT_COMMAND, 0); // Subscribe to individual light control
    mqttClient.subscribe(TOPIC_LIGHT_EFFECT, 0);  // Subscribe to effect control
    mqttClient.subscribe(TOPIC_LIGHT_STOP, 1);    // Subscribe to stop command
    mqttClient.subscribe(TOPIC_CONFIG, 0);        // Subscribe to configuration
    Serial.print("Subscribing at QoS 1");
}

void OnMqttConnect(bool sessionPresent)
{
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
    SuscribeMqtt();
    changeState(OFF_STATE);
}

void OnMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.println("Disconnected from MQTT.");

    if (WiFi.isConnected())
    {
        mqttReconnectTimer.once(2, ConnectToMqtt);
    }
}

void OnMqttSubscribe(uint16_t packetId, uint8_t qos)
{
    Serial.println("Subscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    Serial.print("  qos: ");
    Serial.println(qos);
}

void OnMqttUnsubscribe(uint16_t packetId)
{
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void OnMqttPublish(uint16_t packetId)
{
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

// Function to convert a 4-bit binary string to uint8_t
uint8_t convertBinaryStringToUint8(const char *payload)
{
    // Check if the string is exactly 4 characters long
    if (strlen(payload) != 4)
    {
        // Handle error if necessary (returns 0 by default)
        return 0;
    }

    uint8_t result = 0;

    // Iterate over each character in the string
    for (int i = 0; i < 4; ++i)
    {
        result <<= 1; // Shift `result` one bit to the left

        // Add the corresponding bit (0 or 1)
        if (payload[i] == '1')
        {
            result |= 1;
        }
        else if (payload[i] != '0')
        {
            // Handle incorrect string (return 0 as an example)
            return 0;
        }
    }

    return result;
}

void OnMqttReceived(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    // Get payload if it exists
    if (len != 0)
    {
        payload[len] = '\0'; // Null-terminate the string
        //Serial.println(payload);
    }

    if (strcmp(topic, TOPIC_LIGHT_STOP) == 0)
    {
        stop();
    }
    else if (strcmp(topic, TOPIC_LIGHT_COMMAND) == 0)
    {
        payload[len] = '\0'; // Null-terminate the string
        //Serial.println(payload);
        uint8_t state = convertBinaryStringToUint8(payload);
        changeState(state);
    }
    /*else if (strncmp(topic, "light/", 6) == 0 && strstr(topic, "/command") != NULL)
    {
        int lightIndex = topic[6] - '0'; // Extract the light number
        WebSerial.println(lightIndex);
        if (lightIndex >= 0 && lightIndex < 4)
        {
            WebSerial.print((1 << relayPins[lightIndex]));
            if (strcmp((char *)payload, "1") == 0)
            {
                GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, (1 << relayPins[lightIndex]));
            }
            else if (strcmp((char *)payload, "0") == 0)
            {
                GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, (1 << relayPins[lightIndex]));
            }

            // Disable GPIOs
        }
    }*/
    else if (strcmp(topic, TOPIC_LIGHT_EFFECT) == 0)
    {
        int effect = EFFECT_COUNT;
        int repetitions = 1;
        int delayMs = 200;
        bool invert = false;

        // Parse the payload for effect name, repetitions, delay, and optional invert flag
        char *effectStr = strtok((char *)payload, ",");
        char *repetitionsStr = strtok(NULL, ",");
        char *delayStr = strtok(NULL, ",");
        char *invertStr = strtok(NULL, ",");

        if (effectStr != NULL)
        {
            effect = atoi(effectStr);
        }

        if (repetitionsStr != NULL)
        {
            repetitions = atoi(repetitionsStr);
        }
        if (delayStr != NULL)
        {
            delayMs = atoi(delayStr);
        }
        if (invertStr != NULL && strcmp(invertStr, "invert") == 0)
        {
            invert = true;
        }

        if (repetitions <= 0)
        {
            repetitions = -1; // Infinite loop
        }

        playEffect(effect, repetitions, delayMs, invert);
    }
    else if (strcmp(topic, TOPIC_CONFIG) == 0)
    {
        // Handle configuration
                // Parse the JSON configuration
        StaticJsonDocument<200> doc;
        payload[len] = '\0'; // Null-terminate the string
        DeserializationError error = deserializeJson(doc, payload);

        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }

        // Check and update the legalMode configuration
        if (doc.containsKey("LEGAL_MODE"))
        {
            legalMode = doc["LEGAL_MODE"];
            Serial.print(F("LEGAL_MODE updated to: "));
            Serial.println(legalMode ? "true" : "false");
        }
    }
}

// Publish the current state of a light
void publishState(uint8_t state)
{
    mqttClient.publish(TOPIC_LIGHT_STATE, 0, true, String(state).c_str());
}

AsyncMqttClient *InitMqtt()
{

    mqttClient.onConnect(OnMqttConnect);
    mqttClient.onDisconnect(OnMqttDisconnect);

    mqttClient.onSubscribe(OnMqttSubscribe);
    mqttClient.onUnsubscribe(OnMqttUnsubscribe);

    mqttClient.onMessage(OnMqttReceived);
    mqttClient.onPublish(OnMqttPublish);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    return &mqttClient;
}