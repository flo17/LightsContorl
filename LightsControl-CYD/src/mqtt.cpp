#include "mqtt.hpp"
#include "ESP32_Utils.hpp"
#include "gui.hpp"

TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

AsyncMqttClient mqttClient;
extern void update_connection_status(bool success);
extern void updateLightState(int index, bool state);

// Variables to store light state and state change indicator
uint8_t lightState = 0;
bool stateChanged = false;

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
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        // Connection successful, retrieve IP address

        update_label(message.c_str());
        ConnectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        xTimerStart(wifiReconnectTimer, 0);
        break;
    }
}

void SuscribeMqtt()
{
    uint16_t packetIdSub = mqttClient.subscribe(TOPIC_LIGHT_STATE, 0); // Subscribes to lights topic
    Serial.print("Subscribing at QoS 0, packetId: ");
    Serial.println(packetIdSub);
}

void OnMqttConnect(bool sessionPresent)
{
    Serial.println("Connected to MQTT.");
    update_connection_status(true);
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
    SuscribeMqtt();
}

void OnMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.println("Disconnected from MQTT.");
    update_connection_status(false);

    if (WiFi.isConnected())
    {
        xTimerStart(mqttReconnectTimer, 0);
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

int stringToInt(const char *str)
{
    char *end;
    errno = 0; // Pour détecter les erreurs de conversion
    long value = strtol(str, &end, 10);

    // Vérifier les erreurs de conversion
    if (errno == ERANGE || value > INT_MAX || value < INT_MIN)
    {
        return 0; // Ou une autre valeur par défaut
    }

    if (end == str)
    {
        return 0; // Ou une autre valeur par défaut
    }

    return static_cast<int>(value);
}
void OnMqttReceived(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    if (len != 0)
    {
        payload[len] = '\0'; // Null-terminate the string
    }

    if (strcmp(topic, TOPIC_LIGHT_STATE) == 0)
    {
        // Update the light state and set the stateChanged flag
        lightState = stringToInt(payload);
        stateChanged = true;
    }

    /*elseif (strncmp(topic, "light/", 6) == 0 && strstr(topic, "/state") != NULL)
    {
        int lightNum = topic[6] - '0'; // Extract the light number
        if (lightNum >= 0 && lightNum < 4)
        {
            if (strcmp((char *)payload, STATE_ON) == 0)
            {
                updateLightState(lightNum, STATE_ON);
            }
            else if (strcmp((char *)payload, STATE_OFF) == 0)
            {
                updateLightState(lightNum, STATE_OFF);
            }
        }
    }*/
}

AsyncMqttClient *InitMqtt()
{
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(ConnectToMqtt));
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(ConnectWiFi_STA));

    mqttClient.onConnect(OnMqttConnect);
    mqttClient.onDisconnect(OnMqttDisconnect);

    mqttClient.onSubscribe(OnMqttSubscribe);
    mqttClient.onUnsubscribe(OnMqttUnsubscribe);

    mqttClient.onMessage(OnMqttReceived);
    mqttClient.onPublish(OnMqttPublish);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    return &mqttClient;
}