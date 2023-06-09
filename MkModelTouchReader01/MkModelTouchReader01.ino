/* MKModel Touch Reader PICO 
*  Core 1 - Touch pad polling and response to Master PICO I2C request 
*  Core 2 - Unused
* Parent sketches:
*  MkModelTouchReader01
* MRD 24/04/2023 ver 01
*/
#include <Wire.h>


#define MIN_RETOUCH 5000 // Will not recognise pad re-touch if it occurrs within this time period (milliseconds)

unsigned char noPad = '@'; // represents no pad touched since last master request
unsigned char touchBuffer = noPad; // holds the ID of the last pad that was touched (A - X)
unsigned char prevTouch = noPad;
unsigned long touchTS = 0; // timstamp to prevent accepting retouch (same pad) withn MIN_RETOUCH period

// Polling sequence depends on pcb conector sequence which is not in strict PICO GPIO pin order
const pin_size_t pinRef[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 15, 14, 18, 19, 20, 21, 28, 27, 26, 22, 17, 18, 12, 13 };
const int padCount = sizeof(pinRef) / sizeof(pin_size_t);

void setup() {
  //Serial.begin(9600);
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
  digitalWrite(LED_BUILTIN, LOW);
  for (int index = 0; index < padCount; index++) {
    if (digitalRead(pinRef[index]) == HIGH) {
      digitalWrite(LED_BUILTIN, HIGH);
      while (digitalRead(pinRef[index]) == HIGH) {
        delay(10);
      }
      recordTouch(index);
      break; // dont poll further down the array of touch pads if a touch is detected
    }
  }
}

// record the ID of a touchpad to provision subsequent request from I2C master
void recordTouch(int padIndex) {
  // record 'A' for index 0, 'X' for index 23 etc.
  // recorded value references PCB touchpad connector order not PICO GPIO pin number  
  // Prevent recording the same padIndex within short (5 sec?) time to stop repeated restarting of associated media sequence. Probably not required if image only display.  
  unsigned char touch = padIndex + 'A';
  if(touch != prevTouch || (touch == prevTouch && (millis() > (touchTS + MIN_RETOUCH)))) {
    touchBuffer = touch; 
    prevTouch = touch;
    touchTS = millis(); 
  }
  //Serial.println(touchBuffer);
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
  Wire.write((byte)touchBuffer);
  touchBuffer = noPad;
}
