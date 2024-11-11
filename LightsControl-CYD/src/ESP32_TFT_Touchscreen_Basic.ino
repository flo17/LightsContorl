/*
    Inspired by Rui Santos & Sara Santos - Random Nerd Tutorials
    https://RandomNerdTutorials.com/cyd-lvgl/
*/
#include <lvgl.h>
#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen
#include <XPT2046_Touchscreen.h>

#include <WiFiClient.h>
#include <ElegantOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
// include <PubSubClient.h>
#include <Ticker.h>
#include "credentials.h"
#include "ESP32_Utils.hpp"
#include "mqtt.hpp"
#include "gui.hpp"

// Touchscreen pin configuration
#define XPT2046_IRQ 36  // T_IRQ
#define XPT2046_MOSI 32 // T_DIN
#define XPT2046_MISO 39 // T_OUT
#define XPT2046_CLK 25  // T_CLK
#define XPT2046_CS 33   // T_CS

// Display dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Clients for MQTT and WiFi connections
WiFiClient espClient;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Touchscreen SPI configuration
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Touchscreen coordinate variables: x, y positions and pressure (z)
int x, y, z;

// Display buffer for rendering
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// Calibration function for touchscreen; applies calibration values to raw data
void calibrate_touchscreen(TS_Point &p, int &x, int &y)
{
    float alpha_x = -0.000, beta_x = 0.092, delta_x = -45.107;
    float alpha_y = 0.066, beta_y = 0.000, delta_y = -15.123;

    x = alpha_y * p.x + beta_y * p.y + delta_y;
    x = constrain(x, 0, SCREEN_WIDTH - 1);

    y = alpha_x * p.x + beta_x * p.y + delta_x;
    y = constrain(y, 0, SCREEN_HEIGHT - 1);
}

// Function to read and calibrate touchscreen input
void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (touchscreen.tirqTouched() && touchscreen.touched())
    {
        TS_Point p = touchscreen.getPoint();
        calibrate_touchscreen(p, x, y);
        z = p.z;

        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;

        // Print coordinates and pressure on the Serial Monitor

        /*Serial.print("X = ");
        Serial.print(x);
        Serial.print(" | Y = ");
        Serial.print(y);
        Serial.print(" | Pressure = ");
        Serial.print(z);*/
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// Function to update WiFi and MQTT connection status on label
void update_connection_status(bool success)
{
    String ipStr = WiFi.localIP().toString();
    String message = "IP: " + ipStr + (success ? " MQTT OK" : " NO MQTT");
    update_label(message.c_str());
}

void setup_lvgl()
{
    // Start LVGL
    lv_init();

    // Start the SPI for the touchscreen and init the touchscreen
    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSPI);
    // Set the Touchscreen rotation in landscape mode
    // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
    touchscreen.setRotation(2);

    // Create a display object
    lv_display_t *disp;
    // Initialize the TFT display using the TFT_eSPI library
    disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

    // Initialize an LVGL input device object (Touchscreen)
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    // Set the callback function to read Touchscreen input
    lv_indev_set_read_cb(indev, touchscreen_read);
}



void setup()
{
    Serial.begin(115200);

    // delay(500);
    WiFi.onEvent(WiFiEvent);
    AsyncMqttClient *mqttClient = InitMqtt();

    // Function to draw the GUI (text, buttons and sliders)
    setup_lvgl();
    lv_create_main_gui(mqttClient);

    ConnectWiFi_STA();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Hi! I am ESP8266."); });
    ElegantOTA.begin(&server);
    server.begin();
}

static uint32_t my_tick_get_cb(void) { return millis(); }

void loop()
{
    if (stateChanged)
    {
        for (int i = 0; i < 4; i++)
        {
            uint8_t bitValue = (lightState >> i) & 1;
            updateLightState(i, bitValue);
        }
        stateChanged = false;
    }

    lv_task_handler(); // let the GUI do its work
    lv_tick_set_cb(my_tick_get_cb); // Update the LVGL tick
}
