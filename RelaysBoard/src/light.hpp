#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <stdint.h> // Pour l'utilisation de uint8_t

// GPIO5 (D1), GPIO14 (D5), GPIO12 (D6), GPIO13 (D7)
#define PIN_LIGHT1 13
#define PIN_LIGHT2 12
#define PIN_LIGHT3 14
#define PIN_LIGHT4 5

#define OFF_STATE 0

// Function declarations for light operations
void init_pins();
void stop();
void changeState(uint8_t newState, bool init = false);
void playEffect(int effectName, int repetitions, int delayMs, bool invert);
void updateEffect(); 

#endif // LIGHT_HPP
