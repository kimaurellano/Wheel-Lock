#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Keypad.h>

class InputManager {
   private:
    Keypad &_keypad;
    int charIdx = 0;
    char inputs[64];
    char* inputsPtr = inputs;
    String _invalidChar;
    long unsigned _previousMillis;

   public:
    InputManager(Keypad &keypad);
    bool listen();
    void clear();
    char* getInput() { return inputsPtr; }
};

#endif