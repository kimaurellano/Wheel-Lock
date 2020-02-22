#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>

#define fingerSensorSerial Serial1

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSensorSerial);

LiquidCrystal_I2C lcd(0x27, 16, 2);

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

  // For debugging purposes
  finger.getTemplateCount();
  Serial.print("Sensor contains ");
  Serial.print(finger.templateCount);
  Serial.println(" templates");

  //********************************************LCD
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("WELCOME!"));
  delay(1000);
}

void loop() {
  if (Serial3.available()) {
    Serial.print(Serial3.readString());
    Serial3.print("Hello too");
  }

  if(Serial2.available()){
    Serial.print(F("Message recieved:"));
    Serial.println(Serial2.readString());
  }
}

void SendTo(char *number, String message) {
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