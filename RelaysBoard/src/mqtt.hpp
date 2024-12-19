#ifndef MQTT_HPP
#define MQTT_HPP

#include <AsyncMqttClient.h>
#include <WiFi.h>

// MQTT connection constants
#define MQTT_HOST IPAddress(192, 168, 2, 1) // IP address of the MQTT broker
#define MQTT_PORT 1883                      // MQTT port
#define TOPIC_LIGHT_STATE "light/state"        // Topic for light control
#define TOPIC_LIGHT_COMMAND "light/command"    // Topic for light command
#define TOPIC_LIGHT_EFFECT "light/effect"      // Topic for light effect
#define TOPIC_LIGHT_STOP "light/stop"          // Topic for stopping the effect
#define TOPIC_CONFIG "config"                  // Topic for configuration
#define STATE_OFF "0"                       // State representation for off
#define STATE_ON "1"                        // State representation for on

// Function declarations for MQTT operations
void ConnectWiFi_STA();
AsyncMqttClient* InitMqtt();
void ConnectToMqtt();
void WiFiEvent(WiFiEvent_t event);
void publishState(uint8_t state);

#endif // MQTT_HPP
