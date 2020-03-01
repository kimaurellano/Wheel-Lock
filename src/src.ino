#include <Adafruit_Fingerprint.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>
#include "InputManager.h"

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

Keypad* customKeypad = new Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
InputManager inputManager(*customKeypad);

File userData;

char t[256] = {'\0'};  // This will hold all the numbers
char* tPtr = t;

int otp = 0;
int ackOtp = 0;
bool onUnlock = false;
const int LOCK = 45;

// State machines for sensors.
// Allows synchronous process
long unsigned previousMillisFinger;
long unsigned previousMillisLock;

// reset arduino
void (*resetFunct)(void) = 0;

void setup() {
    // Debug
    Serial.begin(9600);

    pinMode(LOCK, OUTPUT);

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

    // finger.emptyDatabase();

    // For debugging purposes
    finger.getTemplateCount();
    Serial.print(F("Sensor contains "));
    Serial.print(finger.templateCount);
    Serial.println(F(" templates"));

    //********************************************SD Card
    while (!SD.begin(53)) {
        Serial.println(F("SD Error."));
    }

    // SD.remove("userdata.txt");

    // Check for existing file
    if (SD.exists("userdata.txt")) {
        Serial.println(F("file found!"));
    } else {
        // Create the file if non existing
        userData = SD.open("userdata.txt", FILE_WRITE);
        userData.close();

        Serial.println(F("New file has been created"));
    }

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
    long unsigned currentMillis = millis();

    // Finger sensor is idle
    if ((currentMillis - previousMillisFinger) >= 1000) {
        previousMillisFinger = currentMillis;

        if (finger.getImage() == FINGERPRINT_NOFINGER) {
            Serial.print(F("."));
        }
    }

    if ((currentMillis - previousMillisLock) >= 50 && onUnlock) {
        previousMillisFinger = currentMillis;

        digitalWrite(LOCK, HIGH);
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

            int pickedSlot = atoi(inputManager.getInput());
            if (pickedSlot > 5 || pickedSlot < 1) {
                Serial.println(F("Pick slots 1 to 5 only."));
                inputManager.clear();

                return;
            }
            inputManager.clear();
            Serial.println(F("Slot picked."));

            Serial.print(F("\nPlease input Phone#:"));
            while (inputManager.listen())
                ;
            String cellNumber = String(inputManager.getInput());
            Serial.print(F("Phone set:"));
            Serial.println(cellNumber);

            writeToStorage(cellNumber, 1);

            inputManager.clear();

            registerFinger(pickedSlot);

            return;
        } else if (result == FINGERPRINT_OK) {
            /* TODO:
             *  Find which slot the id corresponds
             */
            int fingerId = finger.fingerID;

            verifyUser(fingerId);
        }
    }


}

void verifyUser(int fingerId) {
    userData = SD.open("userdata.txt");
    if (userData) {
        char buffer[128];
        char* bufferPtr = buffer;
        int bufferIdx = 0;

        // Check userdata.txt
        while (userData.available()) {
            char in = userData.read();
            buffer[bufferIdx++] = in;
            if (in == '\n') {
                // Null terminate
                buffer[bufferIdx] = '\0';
                Serial.print(F("Current line:"));
                Serial.println(bufferPtr);

                // Split. This will provide the first split value
                char* pch = strtok(buffer, ",");

                // Remember that the data format within the file
                //          <slot/fingerid>,<phonenumber>
                // element:       0              1
                int element = 0;
                bool exists = false;
                while (pch != nullptr) {
                    if (element == 0 && atoi(pch) == fingerId) {
                        exists = true;
                    }

                    if (exists && element == 1) {
                        otp = generateOTP();

                        // Stream to the bluetooth the otp
                        bluetoothHandShake();

                        sendTo(pch, String(otp));

                        while (!bluetoothAck())
                            ;

                        Serial.print(F("Sent OTP: "));
                        Serial.println(otp);

                        Serial.print(F("Recieved OTP: "));
                        Serial.println(ackOtp);

                        if (ackOtp == otp) {
                            Serial.println("OTP verified.");
                            onUnlock = true;
                        } else {
                            Serial.println("OTP invalid.");
                        }

                        // Avoid unnecessary loop
                        return;
                    }

                    pch = strtok(nullptr, ",");
                    element++;
                }

                Serial.println();

                buffer[128] = {'\0'};
                bufferPtr = buffer;

                bufferIdx = 0;
            }
        }
    }

    userData.close();
}

void writeToStorage(String phoneNumber, int slot) {
    /*  TODO:
     *    Write to file with format: <fingerid>, <phonenumber>
     *    Encrypt the file
     */
    Serial.print(F("Writing..."));

    // ONLY Append the data to the text file
    userData = SD.open("userData.txt", O_RDWR | O_APPEND);
    if (!userData) {
        Serial.println(F("Error writing to SD file."));
        return;
    }

    // Data format: <FingerId> <PhoneNumber> <Slot>
    char buffer[256];
    sprintf(buffer, "%d,%s", slot, phoneNumber.c_str());

    userData.println(buffer);
    userData.close();
    Serial.println(F("Data written successfully."));
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
    // Send random number/OTP to the connected device via HC-05
    Serial3.println(String(otp));
}

bool bluetoothAck() {
    char buffer[6];
    char* bufferPtr = buffer;
    if (Serial3.available()) {
        String ack = Serial3.readString();
        sprintf(buffer, "%s", ack.c_str());

        Serial.print("Recieved ack:");
        Serial.println(buffer);

        ackOtp = atoi(bufferPtr);

        return true;
    }

    return false;
}

int generateOTP() {
    return random(1000, 10000);
}

uint8_t registerFinger(int id) {
    int selectedId = id;
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

    delay(2000);

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
        Serial.println(F("Creating model error."));
        return result;
    }

    Serial.println(F("Storing model"));
    result = finger.storeModel(selectedId);
    if (result != FINGERPRINT_OK) {
        Serial.println(F("Storing model error."));
        return result;
    }

    Serial.println(F("Successfully registered"));
}