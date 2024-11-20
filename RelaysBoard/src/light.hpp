#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <stdint.h> // Pour l'utilisation de uint8_t

// GPIO5 (D1), GPIO14 (D5), GPIO12 (D6), GPIO13 (D7)
#define PIN_LIGHT1 13
#define PIN_LIGHT2 12
#define PIN_LIGHT3 14
#define PIN_LIGHT4 5
#define PIN_RELAY_HB 2 // High beam relay, HIGH at boot
#define PIN_HB_SIGNAL 4 // High beam signal 

#define OFF_STATE 0 // Binary representation 0000
#define HB_STATE 15 // Binary representation 1111
#define DEBOUNCE_TIME 250

extern bool legalMode;

// Function declarations for light operations
void init_pins();
void stop();
void changeState(uint8_t newState, bool init = false);
void playEffect(int effectName, int repetitions, int delayMs, bool invert);
void updateEffect(); 

#endif // LIGHT_HPP
