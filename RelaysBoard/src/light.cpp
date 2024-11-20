#include "light.hpp"
#include "effects.h"
#include "mqtt.hpp"
#include <WebSerial.h>

const int relayPins[4] = {PIN_LIGHT1, PIN_LIGHT2, PIN_LIGHT3, PIN_LIGHT4};
bool stopEffect = false; // Flag to stop all effects

struct Effect
{
    const uint8_t *pattern;
    size_t length;
};

// Define patterns for each effect
const uint8_t blinkingLR[] = {0b1000, 0b0100, 0b0010, 0b0001};
const uint8_t blinkingRL[] = {0b0001, 0b0010, 0b0100, 0b1000};
const uint8_t wave[] = {0b0001, 0b0010, 0b0100, 0b1000, 0b0100, 0b0010, 0b0001};
const uint8_t alternating[] = {0b1010, 0b0101};
const uint8_t blinking[] = {0b1111, 0b0000};
const uint8_t extint[] = {0b1001, 0b0110};
const uint8_t cascadeLR[] = {0b1000, 0b1100, 0b1110, 0b1111};
const uint8_t cascadeRL[] = {0b0001, 0b0011, 0b0111, 0b1111};

unsigned long previousMillis = 0;
size_t patternIndex = 0;
int remainingRepetitions = 0;
bool effectRunning = false;
int delayMs = 0;
int currentEffectName = -1;

unsigned long hbSignalTime = 0;
unsigned long lastHbSignalTime = 0;
bool hbSignal = false;
bool hbState = false;
// By default, the high beam relay is active to respect a legal requirement
bool relayState = true;

// Effects table
Effect effects[EFFECT_COUNT] = {
    {blinkingLR, sizeof(blinkingLR)},
    {blinkingRL, sizeof(blinkingRL)},
    {wave, sizeof(wave)},
    {alternating, sizeof(alternating)},
    {blinking, sizeof(blinking)},
    {extint, sizeof(extint)},
    {cascadeLR, sizeof(cascadeLR)},
    {cascadeRL, sizeof(cascadeRL)},
};

void ICACHE_RAM_ATTR isrhbSignalRising()
{
    hbSignalTime = millis();
    if (hbSignalTime - lastHbSignalTime > DEBOUNCE_TIME)
    {
        // Stop effect to assure the high beam signal is not interrupted
        stopEffect = true;

        hbSignal = true;
        lastHbSignalTime = hbSignalTime;
    }
}

void ICACHE_RAM_ATTR isrhbSignalFalling()
{
    hbSignalTime = millis();
    if (hbSignalTime - lastHbSignalTime > DEBOUNCE_TIME)
    {
        hbSignal = false;
        lastHbSignalTime = hbSignalTime;
    }
}

void init_pins()
{
    pinMode(PIN_HB_SIGNAL, INPUT_PULLUP);
    pinMode(PIN_RELAY_HB, OUTPUT);

    attachInterrupt(PIN_HB_SIGNAL, isrhbSignalRising, RISING);
    attachInterrupt(PIN_HB_SIGNAL, isrhbSignalFalling, FALLING);

    for (int i = 0; i < 4; i++)
    {
        pinMode(relayPins[i], OUTPUT);
    }
    changeState(OFF_STATE, true);
}

void stop()
{
    stopEffect = true;
}

void changeState(uint8_t newState, bool init)
{

    // Enable GPIOs
    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, ((newState & 0b0001) << PIN_LIGHT1) |
                                              ((newState & 0b0010) << PIN_LIGHT2 - 1) |
                                              ((newState & 0b0100) << PIN_LIGHT3 - 2) |
                                              ((newState & 0b1000) << PIN_LIGHT4 - 3)) |
        (relayState << PIN_RELAY_HB);

    // Disable GPIOs
    GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, ((~newState & 0b0001) << PIN_LIGHT1) |
                                              ((~newState & 0b0010) << PIN_LIGHT2 - 1) |
                                              ((~newState & 0b0100) << PIN_LIGHT3 - 2) |
                                              ((~newState & 0b1000) << PIN_LIGHT4 - 3)) |
        (relayState << PIN_RELAY_HB);
    // Publish the new state
    if (init)
    {
        return;
    }
    publishState(newState);
}

void playEffect(int effectName, int repetitions, int delayMsParam, bool invert)
{
    // Initialiser les variables globales
    stopEffect = false;
    changeState(OFF_STATE);
    patternIndex = -1;
    remainingRepetitions = repetitions;
    effectRunning = true;
    previousMillis = millis();
    delayMs = delayMsParam;
    currentEffectName = effectName;
}

// Execute an effect function with repetition and delay, optionally inverted
void updateEffect()
{
    // If the high beam signal is active, turn on the high beam to respect the signal
    if (hbSignal && !hbState)
    {
        changeState(HB_STATE);
        hbState = true;
        return;
    }

    // If the high beam signal is inactive, turn off the high beam
    // A short high beam flash will stop any running effect
    if (!hbSignal && hbState)
    {
        // changeState will be called in the condition below with stopEffect = true
        // changeState(OFF_STATE);
        // hbState = false;

        stopEffect = true;
        return;
    }

    if (stopEffect)
    {
        // If the high beam signal is active, turn on the high beam to respect the signal
        // When a effect is stopped, previous state is restored
        if (hbState)
        {
            changeState(HB_STATE);
            hbState = true;
        }
        else
        {
            changeState(OFF_STATE);
            hbState = false;
        }

        effectRunning = false;
        stopEffect = false;
        return;
    }

    if (!effectRunning)
    {
        return;
    }

    unsigned long currentMillis = millis();

    // If the current effect is out of bounds, stop the effect
    if (currentEffectName < 0 || currentEffectName >= EFFECT_COUNT)
    {
        effectRunning = false;
        changeState(OFF_STATE);
        return;
    }
    // Check if it is the first pattern index
    const uint8_t *pattern = effects[currentEffectName].pattern;
    size_t patternLength = effects[currentEffectName].length;
    uint8_t newState = pattern[patternIndex]; // Retrieve the new state from the pattern

    // Initialize the pattern index if it is the first iteration
    if (patternIndex == -1)
    {
        patternIndex = 0;
        uint8_t newState = pattern[patternIndex]; // Retrieve the new state from the pattern
        changeState(newState);
        patternIndex++;
    }

    // Wait for the delay before updating the state again
    if (currentMillis - previousMillis >= delayMs)
    {
        if (remainingRepetitions == 0)
        {
            effectRunning = false;
            changeState(OFF_STATE);
            return;
        }

        previousMillis = currentMillis;

        if (stopEffect)
        {
            changeState(OFF_STATE);
            effectRunning = false;
            return;
        }

        if (currentEffectName >= 0 && currentEffectName < EFFECT_COUNT)
        {
            changeState(newState);

            patternIndex++;
            if (patternIndex >= patternLength)
            {
                patternIndex = 0;
                if (remainingRepetitions > 0)
                {
                    remainingRepetitions--;
                }
            }
        }
    }
}
