#ifndef ESP32_Utils_HPP
#define ESP32_Utils_HPP

#include <WiFi.h>
#include <lvgl.h>
#include "credentials.h"

// Defining WiFi channel for optimized connection speed
#define WIFI_CHANNEL 6

// Function declarations for MQTT operations
void ConnectWiFi_STA();

#endif // ESP32_Utils_HPP
