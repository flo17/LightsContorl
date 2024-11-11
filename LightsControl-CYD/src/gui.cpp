#include "gui.hpp"
#include "mqtt.hpp"

// Define styles for the light indicators
lv_style_t style_indicator_off;
lv_style_t style_indicator_on;
lv_style_t style_container;
lv_style_transition_dsc_t bg_transition;

extern AsyncMqttClient mqttClient;
bool inv = false; // Inversion state flag

lv_obj_t *repetition_label; // Label for repetitions slider
lv_obj_t *speed_label;      // Label for speed slider
int repetitions = 1;        // Default number of repetitions
int speed = 500;            // Default speed in ms

// Pointer to option label for easier updates
static lv_obj_t *option_label;

// Array to store the light button objects
lv_obj_t *lightButtons[MAX_LIGHTS];
lv_obj_t *label; // Label for displaying connection status

// Effect options array
const char *options[NUM_OPTIONS] = {
    "BlinkingLR",
    "BlinkingRL",
    "Wave",
    "Alternating",
    "Blinking",
    "Ext-int",
    "CascadeLR",
    "CascadeRL"};

// Current option index for effects
static int current_option_index = 0;

void update_label(const char *text)
{
    lv_label_set_text(label, text); // Update label text with IP address
    // lv_task_handler();              // let the GUI do its work
}

// Function to get the current effect option for external use
const char *get_current_option()
{
    return options[current_option_index];
}

// Function to update the label text with the current option
static void update_option_label()
{
    lv_label_set_text(option_label, options[current_option_index]);
}

// Event handler for arrow buttons
static void arrow_event_handler(lv_event_t *e)
{
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    bool isLeftArrow = (bool)lv_event_get_user_data(e);
    current_option_index = (current_option_index + (isLeftArrow ? -1 : 1) + NUM_OPTIONS) % NUM_OPTIONS;
    update_option_label();
}

// GUI setup function for creating effect selector with arrows and label
void create_effect_selector(lv_obj_t *parent)
{
    lv_obj_t *cont_effect_selector = lv_obj_create(parent);
    lv_obj_set_size(cont_effect_selector, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(cont_effect_selector, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_effect_selector, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_grow(cont_effect_selector, 1);
    lv_obj_add_style(cont_effect_selector, &style_container, LV_STATE_DEFAULT);

    // Left arrow button creation
    lv_obj_t *left_arrow = lv_btn_create(cont_effect_selector);
    lv_obj_t *left_label = lv_label_create(left_arrow);
    lv_label_set_text(left_label, "<"); // Left arrow icon
    lv_obj_add_event_cb(left_arrow, arrow_event_handler, LV_EVENT_CLICKED, (void *)true);

    // Label to display current effect option
    option_label = lv_label_create(cont_effect_selector);
    lv_label_set_text(option_label, options[current_option_index]);
    lv_obj_set_style_text_align(option_label, LV_TEXT_ALIGN_CENTER, 0);

    // Right arrow button creation
    lv_obj_t *right_arrow = lv_btn_create(cont_effect_selector);
    lv_obj_t *right_label = lv_label_create(right_arrow);
    lv_label_set_text(right_label, ">"); // Right arrow icon
    lv_obj_add_event_cb(right_arrow, arrow_event_handler, LV_EVENT_CLICKED, (void *)false);

    lv_obj_set_flex_grow(left_arrow, 1);
    lv_obj_set_flex_grow(option_label, 4);
    lv_obj_set_flex_grow(right_arrow, 1);
}

// Callback that prints the current slider value on the TFT display and Serial Monitor for debugging purposes
static void slider_event_rep_callback(lv_event_t *e)
{
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);

    repetitions = (int)lv_slider_get_value(obj);

    lv_label_set_text_fmt(label, "Repetitions: %d", repetitions);
}

static void slider_event_speed_callback(lv_event_t *e)
{
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);

    int val = (int)lv_slider_get_value(obj);

    // Calculate the speed based on the step value
    speed = (val / SPEED_STEP) * SPEED_STEP;

    // Update slider and label values to match interval
    lv_slider_set_value(obj, speed, LV_ANIM_OFF);
    lv_label_set_text_fmt(label, "Speed: %d ms", speed);
}

// Create sliders for effect repetitions and speed
void create_sliders(lv_obj_t *parent)
{
    lv_obj_t *cont_repetition = lv_obj_create(parent);
    lv_obj_set_size(cont_repetition, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont_repetition, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_repetition, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_grow(cont_repetition, 1);
    lv_obj_add_style(cont_repetition, &style_container, LV_STATE_DEFAULT);

    // Repetition label
    repetition_label = lv_label_create(cont_repetition);
    lv_label_set_text_fmt(repetition_label, "Repetitions:%d", repetitions);

    // Create slider for repetitions
    lv_obj_t *slider_rep = lv_slider_create(cont_repetition);
    lv_slider_set_range(slider_rep, 0, MAX_REPETITIONS);

    // lv_obj_align_to(repetition_label, slider_rep, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_set_flex_grow(repetition_label, 1);
    lv_obj_set_flex_grow(slider_rep, 1);

    // Speed
    lv_obj_t *cont_speed = lv_obj_create(parent);
    lv_obj_set_size(cont_speed, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont_speed, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_speed, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_grow(cont_speed, 1);
    lv_obj_add_style(cont_speed, &style_container, LV_STATE_DEFAULT);

    // Speed label
    speed_label = lv_label_create(cont_speed);
    lv_label_set_text_fmt(speed_label, "Speed:%d ms", speed);

    // Create slider for speed
    lv_obj_t *slider_speed = lv_slider_create(cont_speed);
    lv_slider_set_range(slider_speed, MIN_SPEED, MAX_SPEED);
    // lv_obj_align_to(speed_label, slider_speed, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_set_flex_grow(speed_label, 1);
    lv_obj_set_flex_grow(slider_speed, 1);

    // Add callback for sliders
    lv_obj_add_event_cb(slider_speed, slider_event_speed_callback, LV_EVENT_VALUE_CHANGED, speed_label);
    lv_obj_add_event_cb(slider_rep, slider_event_rep_callback, LV_EVENT_VALUE_CHANGED, repetition_label);

    // Set default value
    lv_slider_set_value(slider_speed, speed, LV_ANIM_OFF);
    lv_slider_set_value(slider_rep, repetitions, LV_ANIM_OFF);
}

static void start_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED)
    {
        // Use the index in the topic string for each light
        char payload[32];
        snprintf(payload, sizeof(payload), "%i,%i,%i,%i", current_option_index, repetitions, speed, inv);
        mqttClient.publish("light/effect", 0, false, payload);
    }
}

static void stop_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED)
    {
        LV_LOG_USER("Toggled stop button");
        mqttClient.publish("light/stop", 1, false, NULL);
    }
}

void create_command_buttons(lv_obj_t *parent)
{
    lv_obj_t *cont_effect_buttons = lv_obj_create(parent);
    lv_obj_set_size(cont_effect_buttons, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont_effect_buttons, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_effect_buttons, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START);
    lv_obj_set_flex_grow(cont_effect_buttons, 1);
    lv_obj_add_style(cont_effect_buttons, &style_container, LV_STATE_DEFAULT);

    // Start button
    lv_obj_t *btn_start = lv_btn_create(cont_effect_buttons);
    lv_obj_add_event_cb(btn_start, start_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_width(btn_start, 140);
    lv_obj_set_style_bg_color(btn_start, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(btn_start, LV_ALIGN_CENTER, +80, -20);
    lv_obj_remove_flag(btn_start, LV_OBJ_FLAG_PRESS_LOCK);

    lv_obj_t *label_start = lv_label_create(btn_start);
    lv_label_set_text(label_start, "Start");
    lv_obj_center(label_start);

    // Stop button
    lv_obj_t *btn_stop = lv_btn_create(cont_effect_buttons);
    lv_obj_add_event_cb(btn_stop, stop_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_width(btn_stop, 140);
    lv_obj_set_style_bg_color(btn_stop, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align(btn_stop, LV_ALIGN_CENTER, -80, -20);
    lv_obj_remove_flag(btn_stop, LV_OBJ_FLAG_PRESS_LOCK);

    lv_obj_t *label_stop = lv_label_create(btn_stop);
    lv_label_set_text(label_stop, "Stop");
    lv_obj_center(label_stop);
}

static void inv_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        inv = lv_obj_has_state(obj, LV_STATE_CHECKED) ? true : false;
    }
}

// Update the state and style of each light indicator
void updateLightState(int index, bool state)
{
    if (state)
    {
        lv_obj_add_state(lightButtons[index], LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(lightButtons[index], LV_STATE_CHECKED);
    }
}

static void event_handler_btnm(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);

    if (code == LV_EVENT_PRESSED)
    {
        uint32_t id = lv_btnmatrix_get_selected_btn(obj);
        const char *txt = lv_btnmatrix_get_btn_text(obj, id);
        mqttClient.publish(TOPIC_LIGHT_COMMAND, 0, false, txt);
    }
}

void create_light_control(lv_obj_t *parent)
{
    lv_obj_update_layout(parent);
    static const char *btnm_map[] = {"0000", "1111", "\n",
                                     "1001", "0110", "\n",
                                     "1100", "0011", ""};

    lv_obj_t *btnm1 = lv_buttonmatrix_create(parent);
    lv_obj_set_width(btnm1, lv_pct(100));
    lv_buttonmatrix_set_map(btnm1, btnm_map);
    lv_obj_update_layout(btnm1);
    lv_obj_set_flex_grow(btnm1, 1);
    lv_obj_add_style(btnm1, &style_container, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btnm1, event_handler_btnm, LV_EVENT_ALL, NULL);
}

// Callback that is triggered when light is clicked/toggled
static void event_handler_light(lv_event_t *e)
{
    int *index = (int *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);

    if (code == LV_EVENT_PRESSED)
    {
        LV_UNUSED(obj);
        LV_LOG_USER("Toggled %d %s", *index, lv_obj_has_state(obj, LV_STATE_CHECKED) ? "on" : "off");

        // Read the current state of the lights
        uint8_t newState = lightState;

        // Update the bit corresponding to the pressed light
        if (lv_obj_has_state(obj, LV_STATE_CHECKED))
        {
            newState &= ~(1 << *index); // Turn off the bit
        }
        else
        {
            newState |= (1 << *index); // Turn on the bit
        }

        // Update the global light state
        // lightState = newState;
        // stateChanged = true;

        // Publish the updated state to the MQTT topic
        char payload[5];
        for (int i = 0; i < 4; i++)
        {
            payload[3 - i] = (newState & (1 << i)) ? '1' : '0';
        }
        payload[4] = '\0'; // Null-terminate the string
        Serial.println(payload);
        mqttClient.publish(TOPIC_LIGHT_COMMAND, 0, true, payload);
    }
}

void create_light_indicators(lv_obj_t *parent)
{
    lv_obj_t *cont_lights = lv_obj_create(parent);
    lv_obj_set_size(cont_lights, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont_lights, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_lights, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START);
    lv_obj_set_flex_grow(cont_lights, 1);
    lv_obj_add_style(cont_lights, &style_container, LV_STATE_DEFAULT);

    int x_start = 32;   // Starting x position for the light indicator
    int x_spacing = 64; // Horizontal spacing between each indicator
    for (int i = MAX_LIGHTS - 1; i >= 0; i--)
    {
        int *index_ptr = (int *)malloc(sizeof(int));
        *index_ptr = i;

        lightButtons[i] = lv_obj_create(cont_lights);
        // lv_obj_set_pos(lightButtons[i], x_start + (i * x_spacing), 10);
        lv_obj_add_event_cb(lightButtons[i], event_handler_light, LV_EVENT_PRESSED, index_ptr);
        lv_obj_set_size(lightButtons[i], 60, 60);
        lv_obj_add_style(lightButtons[i], &style_indicator_off, LV_STATE_DEFAULT);
        lv_obj_add_style(lightButtons[i], &style_indicator_on, LV_STATE_CHECKED);
    }
}

void create_status_label(lv_obj_t *parent)
{
    // Créer un label pour afficher "Connecting..."
    label = lv_label_create(parent);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void define_styles()
{
    // Define styles for light indicators
    lv_style_init(&style_indicator_off);
    lv_style_set_radius(&style_indicator_off, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&style_indicator_off, lv_palette_main(BG_COLOR_OFF));
    lv_style_set_transition(&style_indicator_off, &bg_transition);

    lv_style_init(&style_indicator_on);
    lv_style_set_radius(&style_indicator_on, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&style_indicator_on, lv_palette_main(BG_COLOR_ON));
    lv_style_set_transition(&style_indicator_on, &bg_transition);

    // Applique uniquement les propriétés souhaitées
    lv_style_init(&style_container);
    lv_style_set_pad_all(&style_container, 0);            // Pas de marges
    lv_style_set_border_width(&style_container, 0);       // Pas de bordure
    lv_style_set_bg_opa(&style_container, LV_OPA_TRANSP); // Arrière-plan transparent
}

void lv_create_main_gui(void *mqttClient)
{
    mqttClient = (AsyncMqttClient *)mqttClient;
    /*Create a Tab view object*/
    
    lv_obj_t *tabview = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(tabview, 30);

    // content
    lv_obj_t *content = lv_tabview_get_content(tabview);
    lv_obj_set_style_layout(content, LV_LAYOUT_FLEX, LV_PART_MAIN | LV_STATE_DEFAULT);

    /*Add 3 tabs (the tabs are page (lv_page) and can be scrolled*/
    lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "Feux");
    lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "Effects");
    lv_obj_t *tab3 = lv_tabview_add_tab(tabview, "PV");

    lv_tabview_set_active(tabview, 2, LV_ANIM_OFF);
    // Define style transition
    static const lv_style_prop_t transition_props[] = {LV_STYLE_BG_COLOR, LV_STYLE_PROP_INV};                          // Properties to animate
    lv_style_transition_dsc_init(&bg_transition, transition_props, lv_anim_path_linear, TRANSITION_DURATION, 0, NULL); // Set the transition and duration
    lv_obj_remove_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);

    define_styles();

    lv_obj_t *cont_tab1 = lv_obj_create(tab1);
    lv_obj_remove_style_all(cont_tab1);
    lv_obj_set_flex_flow(cont_tab1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(cont_tab1, lv_pct(100), lv_pct(100));
    create_light_indicators(cont_tab1);
    create_light_control(cont_tab1);

    /*Create a container with ROW flex direction*/
    lv_obj_t *cont_tab2 = lv_obj_create(tab2);
    lv_obj_remove_style_all(cont_tab2);
    lv_obj_set_flex_flow(cont_tab2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(cont_tab2, lv_pct(100), lv_pct(100));

    create_effect_selector(cont_tab2);
    create_sliders(cont_tab2);

    // Create checkbox for effect inversion
    lv_obj_t *checkbox_39 = lv_checkbox_create(cont_tab2);
    lv_checkbox_set_text(checkbox_39, "Inv");
    lv_obj_add_event_cb(checkbox_39, inv_handler, LV_EVENT_VALUE_CHANGED, NULL);

    create_command_buttons(cont_tab2);

    create_status_label(tab3);
}