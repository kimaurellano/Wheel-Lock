#include "InputManager.h"
#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define fingerSensorSerial Serial1

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSensorSerial);

LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;  // four rows
    const byte COLS = 4;  // four columns
    // define the cymbols on the buttons of the keypads
    char hexaKeys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                                 {'4', '5', '6', 'B'},
                                 {'7', '8', '9', 'C'},
                                 {'*', '0', '#', 'D'}};
    byte rowPins[ROWS] = {5, 4, 3, 2};  // connect to the row pinouts of the keypad
    byte colPins[COLS] = {9, 8, 7, 6};  // connect to the column pinouts of the keypad

Keypad *customKeypad = new Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
InputManager inputManager(*customKeypad);
// InputManager inputManager(*customKeypad);

int charIdx = 0;
int pickedSlot = 0;

long otp = 0;

// reset arduino
void (*resetFunct)(void) = 0;

void setup() {
    // Debug
    Serial.begin(9600);

    //********************************************HC-05 sensor
    Serial3.begin(9600);

    //********************************************GSM
    Serial2.begin(19200);

    //********************************************Finger sensor
    finger.begin(57600);

    // Give time for the finger sensor to process
    delay(5);

    if (finger.verifyPassword()) {
        Serial.println(F("Finger sensor detected."));
    } else {
        Serial.println(F("Finger sensor not found."));
    }

    finger.emptyDatabase();

    // For debugging purposes
    finger.getTemplateCount();
    Serial.print(F("Sensor contains "));
    Serial.print(finger.templateCount);
    Serial.println(F(" templates"));

    //********************************************LCD
    // lcd.init();
    // lcd.backlight();

    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print(F("WELCOME!"));
    // delay(1000);

    Serial.println(F("Started."));
}

void loop() {
    // Finger sensor is idle
    while (finger.getImage() == FINGERPRINT_NOFINGER) {
        Serial.print(F("."));
    }

    // Someone tapped
    if (finger.getImage() == FINGERPRINT_OK) {
        Serial.println(F("finger scanned!"));
        uint8_t result = finger.image2Tz();
        if (result != FINGERPRINT_OK) {
            Serial.println(result);
            return;
        }
        Serial.println(F("finger image taken!"));

        result = finger.fingerFastSearch();

        //********************************************Registration
        if (result == FINGERPRINT_NOTFOUND) {
            Serial.println(F("finger is currently not registered!"));
            Serial.print(F("Pick slot(1-5):"));

            while (inputManager.listen())
                ;

            Serial.println(F("DONE"));

        } else if (result == FINGERPRINT_OK) {
            /* TODO:
             *  Find which slot the id corresponds
             */
        }
    }
}

char* registerCellNumber() {
    // char cellNumber[15];
    // char* cellNumberPtr = cellNumber;

    // while (true) {
    //     char customKey = customKeypad.getKey();

    //     // Accept only digit and '#' as enter key
    //     if (isAlpha(customKey)) {
    //         continue;
    //     }

    //     if (customKey == '*') {
    //         clearInput();
    //     }

    //     cellNumber[charIdx++] = customKey;

    //     if (customKey == '#') {
    //         return cellNumberPtr;
    //     }
    // }
}

void writeToStorage(char* number, char* fingerId) {
    /*  TODO:
     *    Write to file with format: <fingerid>, <phonenumber>
     *    Encrypt the file
     */
}

void sendTo(char* number, String message) {
    *(number + 10) = '\0';

    Serial.println("-------------------");
    Serial.println(number);
    Serial2.print(F("AT+CMGS =\""));
    Serial2.print(number);
    Serial2.print(F("\"\r\n"));
    delay(1000);
    Serial2.println(message);
    delay(1000);
    Serial2.println((char)26);
    delay(1000);
}

void bluetoothHandShake() {
    // Generate a 4 digit random number
    otp = random(1000, 10000);

    // Send random number/OTP to the connected device via HC-05
    Serial3.write(otp);
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintID() {
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) return -1;

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) return -1;

    p = finger.fingerFastSearch();
    if (p != FINGERPRINT_OK) return -1;

    // found a match!
    Serial.print(F("Found ID #"));
    Serial.print(finger.fingerID);
    Serial.print(F(" with confidence of "));
    Serial.println(finger.confidence);
    return finger.fingerID;
}

uint8_t registerFinger(char* id) {
    int selectedId = atoi(id);
    Serial.print(F("Selected slot:"));
    Serial.println(selectedId);

    int result = -1;

    Serial.println(F("Waiting for valid finger to register"));
    while (result != FINGERPRINT_OK) {
        result = finger.getImage();
        switch (result) {
            case FINGERPRINT_OK:
                Serial.println(F("Image taken"));
                break;
            case FINGERPRINT_NOFINGER:
                Serial.print(F("."));
                break;
        }
    }

    result = finger.image2Tz(1);
    if (result != FINGERPRINT_OK) {
        return result;
    }
    Serial.println(F("Image converted\n"));
    Serial.println(F("Remove finger\n"));

    delay(5000);

    result = 0;
    while (result != FINGERPRINT_NOFINGER) {
        result = finger.getImage();
    }

    result = -1;
    Serial.println(F("Place same finger again"));
    while (result != FINGERPRINT_OK) {
        result = finger.getImage();
        if (result == FINGERPRINT_OK) {
            break;
        }
    }

    result = finger.image2Tz(2);
    if (result != FINGERPRINT_OK) {
        return result;
    }

    Serial.print(F("Creating model for #"));
    Serial.println(selectedId);

    result = finger.createModel();
    if (result != FINGERPRINT_OK) {
        return result;
    }

    Serial.println(F("Storing model"));
    result = finger.storeModel(selectedId);
    if (result != FINGERPRINT_OK) {
        return result;
    }

    Serial.println(F("Successfully registered"));

    // Avoid overwriting the last input to the current user input
    // clearInput();
}

// void clearInput() {
//     memset(&input[0], 0, sizeof(input));

//     input[20] = {'\0'};
//     inputPtr = input;

//     charIdx = 0;

//     Serial.println(F("Cache cleared."));
// }