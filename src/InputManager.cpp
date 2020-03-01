#include <Arduino.h>
#include <Keypad.h>
#include "InputManager.h"

InputManager::InputManager(Keypad &keypad) : _keypad(keypad) {
    _keypad = keypad;
}

bool InputManager::listen() {
    long unsigned currentMillis = millis();
    if ((currentMillis - _previousMillis) >= 50) {
        _previousMillis = currentMillis;

        char c = _keypad.getKey();

        // Enter key
        if (c == '#') {
            Serial.println("\nFinalized.");
            Serial.println(inputsPtr);

            return false;
        }

        // Non-enter key input means user is typing
        if (c && c != '#' && c != '*') {
            inputs[charIdx++] = c;

            Serial.print(c);
        }

        if(c == '*'){
            clear();
        }
    }

    return true;
}

void InputManager::clear() {
    memset(&inputs[0], 0, sizeof(inputs));

    inputs[20] = {'\0'};
    inputsPtr = inputs;

    charIdx = 0;

    Serial.println(F("Cache cleared."));
}