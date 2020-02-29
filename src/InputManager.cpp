#include <Arduino.h>
#include <Keypad.h>
#include "InputManager.h"

InputManager::InputManager(Keypad &keypad) : _keypad(keypad) {
    _keypad = keypad;
}

void InputManager::listen() {
    long unsigned currentMillis = millis();
    if ((currentMillis - _previousMillis) >= 50) {
        _previousMillis = currentMillis;

        char c = _keypad.getKey();

        // Enter key
        if (c == '#') {
            Serial.println("Finalized.");

            // Avoid appending the last input session to the curren session
            clear();
        }

        // Non-enter key input means user is typing
        if (c && c != '#' && c != '*') {
            inputs[charIdx++] = c;

            Serial.println(c);
        }

        if(c == '*'){
            clear();
        }
    }
}

void InputManager::clear() {
    memset(&inputs[0], 0, sizeof(inputs));

    inputs[20] = {'\0'};
    inputsPtr = inputs;

    charIdx = 0;

    Serial.println(F("Cache cleared."));
}

char* InputManager::capturedInput(){
    return inputsPtr;
}