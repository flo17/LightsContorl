#ifndef GUI_HPP
#define GUI_HPP

#include <lvgl.h> // Inclure la bibliothèque LVGL pour utiliser les types et fonctions LVGL
#include <AsyncMqttClient.h>

// Background colors for light indicators
#define BG_COLOR_OFF LV_PALETTE_GREY
#define BG_COLOR_ON LV_PALETTE_AMBER

// Maximum light indicators and parameters for effects
#define MAX_LIGHTS 4
#define MAX_REPETITIONS 20
#define MIN_SPEED 500
#define MAX_SPEED 2000

#define NUM_OPTIONS 8

#define COMMAND_OFF "0" // Command for off state
#define COMMAND_ON "1"  // Command for on state

// Durée de la transition
#define TRANSITION_DURATION 100 // Transition animation duration
#define SPEED_STEP 100        // Step for speed slider

// Prototypes des fonctions et variables externes si nécessaire
void lv_create_main_gui(void *mqttClient);
void update_label(const char *text);
void updateLightState(int index, bool state);

#endif // GUI_HPP
