#ifndef MQTT_HPP
#define MQTT_HPP

#include <AsyncMqttClient.h>
#include <WiFi.h>
#include <lvgl.h>

// MQTT connection constants
#define MQTT_HOST IPAddress(192, 168, 2, 1) // IP address of the MQTT broker
#define MQTT_PORT 1883                      // MQTT port
#define TOPIC_LIGHT_STATE "light/state"     // Topic for light control
#define TOPIC_LIGHT_COMMAND "light/command" // Topic for light command
#define TOPIC_LIGHT_EFFECT "light/effect"   // Topic for light effect
#define TOPIC_LIGHT_STOP "light/stop"       // Topic for stopping the effect
#define TOPIC_CONFIG "config"               // Topic for configuration
#define STATE_OFF "0"                       // State representation for off
#define STATE_ON "1"                        // State representation for on

// Function declarations for MQTT operations
AsyncMqttClient *InitMqtt();
void ConnectToMqtt();
void WiFiEvent(WiFiEvent_t event);
void OnMqttConnect(bool sessionPresent);
void OnMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void OnMqttReceived(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

// Variables to store light state and state change indicator
extern uint8_t lightState;
extern bool stateChanged;

#endif // MQTT_HPP
