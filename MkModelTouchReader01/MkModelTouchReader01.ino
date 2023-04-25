/* MKModel Touch Reader PICO 
*  Core 1 - Touch pad polling and response to Master PICO I2C request 
*  Core 2 - Unused
* Parent sketches:
*  MkModelTouchReader01
* MRD 24/04/2023 ver 01
*/
#include <Wire.h>

#define TOUCH_PIN 28
#define MAX_TOUCH_HOLD 5000

unsigned char noPad = '@';
unsigned char lastTouch = noPad;
// Polling sequence depends on boar conection which are not in strict PICO GPIO pin order
const pin_size_t pinRef[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 15, 14, 18, 19, 20, 21, 28, 27, 26, 22, 17, 18, 12, 13 };
const int padCount = sizeof(pinRef) / sizeof(pin_size_t);

void setup() {
  //Serial.begin(9600);
  //while(!Serial){}
  //Serial.println("Initialising");
  pinMode(LED_BUILTIN, OUTPUT);
  // Set touchpad GPIO pin modes
  initTouchPads();
  // initialaise I2C communincations
  initWire();
  //Serial.println("Ready");
}

void loop() {
  readTouchPads();
}

void initTouchPads(){
  // initialise touch pad GPIO pins
  for (int index = 0; index < padCount; index++) {
    pinMode(pinRef[index],  INPUT_PULLDOWN);
    //Serial.println(pinRef[index]);
  }
}

void initWire(){
   // initialise the I2C comms
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin(0x30);
  Wire.onReceive(recv);
  Wire.onRequest(req);
  delay(100);
}

void readTouchPads() {
  for (int index = 0; index < padCount; index++) {
    long touchRelease = 0;
    if (digitalRead(pinRef[index]) == HIGH) {
      digitalWrite(LED_BUILTIN, HIGH);
      touchRelease = millis();
      while (digitalRead(pinRef[index]) == HIGH) {
        if (millis() > touchRelease + MAX_TOUCH_HOLD) {
          break;
        }
      }
      digitalWrite(LED_BUILTIN, LOW);
      recordTouch(index);
      break;
    }
  }
}

void recordTouch(int padIndex) {
  // record 'A' for index 0, 'X' for index 23 etc.
  // recorded value refernces PCB touchpad connector order not PICO GPIO pin number  
  lastTouch = padIndex + 'A';
  //Serial.println(lastTouch);
}

// Called when the I2C slave gets written to
void recv(int len) {
  // Just consume anything recieved for now
  for (int i = 0; i < len; i++) {
    Wire.read();
  }
}

// Called when the I2C slave is read from
void req() {
  Wire.write((byte)lastTouch);
  lastTouch = noPad;
}
