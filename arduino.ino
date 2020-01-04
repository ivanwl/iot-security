#include <Wire.h>
#include <Adafruit_VC0706.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>

#define BUZZER_PIN 8
#define chipSelect 10
#if ARDUINO >= 100
SoftwareSerial cameraconnection = SoftwareSerial(2, 3);
#else
NewSoftSerial cameraconnection = NewSoftSerial(2, 3);
#endif
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);


bool flag1 = LOW;
char recieveData[32];
byte sendData[32];
uint8_t* imageData = NULL;
bool sendingImage = false;
bool arduino = true;
bool buzzer = true;
bool motion = true;
long buzzerTimer;
uint8_t bytesToRead;

uint16_t jpglen = 0;

void cameraSetup() {
#if !defined(SOFTWARE_SPI)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if (chipSelect != 53) pinMode(53, OUTPUT); // SS on Mega
#else
  if (chipSelect != 10) pinMode(10, OUTPUT); // SS on Uno, etc.
#endif
#endif
}

void i2cSetup() {
  Wire.begin(0x08);
  pinMode(13, OUTPUT);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(sendEvent);
}

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  cameraSetup();
  Serial.begin(9600);

  if (cam.begin()) {
    Serial.println("Camera Found:");
  } else {
    Serial.println("No camera found?");
    return;
  }
  char *reply = cam.getVersion();
  if (reply == 0) {
    Serial.print("Failed to get version");
  } else {
    Serial.println("-----------------");
    Serial.print(reply);
    Serial.println("-----------------");
  }

  //cam.setImageSize(VC0706_640x480);        // biggest
  //cam.setImageSize(VC0706_320x240);        // medium
  cam.setImageSize(VC0706_160x120);          // small

  /*
    uint8_t imgsize = cam.getImageSize();
    Serial.print("Image size: ");
    if (imgsize == VC0706_640x480) Serial.println("640x480");
    if (imgsize == VC0706_320x240) Serial.println("320x240");
    if (imgsize == VC0706_160x120) Serial.println("160x120");
  */

  i2cSetup();

  Serial.println("Setup Done");
}

void resetBuzzer() {
  if (buzzerTimer + 2000 < millis()) {
    buzzerTimer = 0;
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void clearImageData() {
  imageData = NULL;
  jpglen = 0;
  sendingImage = false;
}

//sends up to 30 characters at a time
void sendToESP(byte* data) {
  int i = 0;
  for (i = 0; i < sizeof(data) + 1; ++i)
    sendData[i] = data[i];
  sendData[i] = '!';
}

void sendToESP(String data) {
  int i = 0;
  for (; i < data.length(); ++i)
    sendData[i] = data[i];
  sendData[i] = '!';
  sendData[i + 1] = '\0';
}

void loop()
{
  if (flag1 == HIGH)
  {
    //Serial.println(recieveData);
    flag1 = LOW;
  }
  digitalWrite(13, !digitalRead(13));

  resetBuzzer();
  if (arduino) {
    if (motion)
      cam.setMotionDetect(true);
    else
      cam.setMotionDetect(false);
    /*
      Serial.print("Motion detection is ");
      if (cam.getMotionDetect())
      Serial.println("ON");
      else
      Serial.println("OFF");
    */
    //Serial.println(jpglen);
    //Serial.print("sendingimage "); Serial.println(sendingImage);
    if (jpglen <= 0 && (!motion || cam.motionDetected())) {
      cam.setMotionDetect(false);
      motion = true;

      //capture delay
      if (! cam.takePicture())
        Serial.println("Failed to snap!");
      else
        Serial.println("Picture taken!");

      jpglen = cam.frameLength();
      Serial.print(jpglen, DEC); Serial.println(" byte image");

      if (buzzer && buzzerTimer == 0) {
        digitalWrite(BUZZER_PIN, HIGH);
        buzzerTimer = millis();
      }

      uint8_t *buffer;
      bytesToRead = min(32, jpglen);
      buffer = cam.readPicture(bytesToRead);
      imageData = buffer;
      Serial.println(jpglen);

    }

    if (jpglen > 0 && imageData == NULL) {
      uint8_t *buffer;
      bytesToRead = min(32, jpglen);
      buffer = cam.readPicture(bytesToRead);
      imageData = buffer;
      Serial.println(jpglen);
      //Serial.println(*imageData);
    }
    /*
        if (jpglen <= 0) {
          clearImageData();
        }
    */
  }

  memset(recieveData, '\0', sizeof(recieveData));
  //delay(100);
}

void receiveEvent(int howmany)
{
  for (int i = 0; i < howmany; ++i)
    recieveData[i] = Wire.read();
  if (recieveData[0] == 'a') {
    if (recieveData[1] == '1') {
      arduino = true;
    }
    else if (recieveData[1] == '0') {
      arduino = false;
      clearImageData();
      digitalWrite(BUZZER_PIN, LOW);
      buzzerTimer = 0;
    }
  } else if (recieveData[0] == 'b') {
    if (recieveData[1] == '1')
      buzzer = true;
    else if (recieveData[1] == '0')
      buzzer = false;
  } else if (recieveData[0] == 'c') {
    motion = false;
    clearImageData();
  }
  flag1 = HIGH;
}

//keep function short to stay within interrupt window
void sendEvent()
{
  if (!sendingImage) {
    sendToESP(String(jpglen));
    Wire.write(sendData, 32);
    if (jpglen > 0) {
      sendingImage = true;
    }
  } else {
    Wire.write(imageData, bytesToRead);
    jpglen -= bytesToRead;
    //Serial.print("Remaining len: "); Serial.println(jpglen);
    imageData = NULL;
    if (jpglen <= 0)
      sendingImage = false;
  }

  memset(sendData, '!', sizeof(sendData));
}
