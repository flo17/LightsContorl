#include "ESP32_Utils.hpp"

extern lv_obj_t *label;

void ConnectWiFi_STA()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
    String message = "Connecting to WiFi..."; // Create message with IP
    LV_LOG_USER(message.c_str());
    lv_label_set_text(label, message.c_str());
    lv_task_handler(); // let the GUI do its work

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        vTaskDelay(500 / portTICK_PERIOD_MS); // Délai pour éviter un CPU à 100%
    }
    LV_LOG_USER("Connected to WiFi");
}