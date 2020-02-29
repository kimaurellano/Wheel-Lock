#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Keypad.h>

class InputManager {
   private:
    Keypad &_keypad;
    int charIdx = 0;
    char inputs[10];
    char* inputsPtr = inputs;
    String _invalidChar;
    long unsigned _previousMillis;

    void clear();
   public:
    InputManager(Keypad &keypad);
    void listen();
    char* capturedInput();
};

#endif