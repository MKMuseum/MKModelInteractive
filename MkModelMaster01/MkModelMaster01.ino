
/* MKModel Master PICO 
*  Core 1 - Comms with touch sensor slave PICO (I2C) and DF12015 soundcard (serial)
*  Core 2 - lamp control via TLC59711 (SPI) 
*     (Based on the Adafruit TLC59711 PWM/LED driver library, authors: Limor Fried & Ladyada).
* Parent sketches:
*   MkModelLampManagerSPI03 (TLC59711 control via SPI)
*   TouchPadTestI2Smaster01 (Pico - Pico comms via I2C)
*   DF1201S_Test (DF12015 control via serial)
* MRD 24/04/2023 ver 01
*/

//#include <SPI.h>
#include <SPI.h>
#include <Arduino.h>
#include <DFRobot_DF1201S.h>
#include <SoftwareSerial.h>
#include <Wire.h>

// General constants
#define HIGHLIGHT_TIMEOUT 60000
#define RETOUCH_DELAY 100

// Constants and Globals for PICO - PICO I2C comms
const char noPad = '@';

// Definitions and global objects DF12015
#define VOLUME 10
#define TRACK 4
SoftwareSerial DF1201SSerial(4, 5);  //RX  TX
DFRobot_DF1201S DF1201S;

// TLC9711 variables and constants
#define DATAIN 16          // not used by TLC59711 breakout
#define CHIPSELECT 17      // not used by TLC59711 breakout
#define SPICLOCK 18        // ci on TLC59711 breakout
#define DATAOUT 19         // di on TLC59711 breakout
#define SPI_FREQ 14000000  // SPI frequency 
#define LED_BRIGHT 0x7F
#define RAMP_UP 255
#define RAMP_DOWN -255
#define MIN_PWM 0
#define MAX_PWM 65535

const int8_t numdrivers = 1;  // number of chained breakout boards
const int8_t numChannels = 12 * numdrivers;
uint8_t BCr = LED_BRIGHT;
uint8_t BCg = LED_BRIGHT;
uint8_t BCb = LED_BRIGHT;
// Array of struct to hold current pwm and ramp direction for each channel
struct channelState {
  long pwm;
  int ramp;
} channelStates[numChannels];

// TLC9711 functions
// Initialise SPI for PI PICO
void setupSPI() {
  pinMode(SPICLOCK, OUTPUT);
  pinMode(DATAOUT, OUTPUT);
  SPI.setSCK(SPICLOCK);
  SPI.setTX(DATAOUT);
  delay(5);
  SPI.begin();
  delay(5);
}

// Write channel pwm info to TLC59711
void tlcWrite() {
  uint32_t command;

  // Magic word for write
  command = 0x25;

  command <<= 5;
  // OUTTMG = 1, EXTGCK = 0, TMGRST = 1, DSPRPT = 1, BLANK = 0 -> 0x16
  command |= 0x16;

  command <<= 7;
  command |= BCr;

  command <<= 7;
  command |= BCg;

  command <<= 7;
  command |= BCb;

  SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
  for (uint8_t n = 0; n < numdrivers; n++) {
    SPI.transfer(command >> 24);
    SPI.transfer(command >> 16);
    SPI.transfer(command >> 8);
    SPI.transfer(command);

    // 12 channels per TLC59711
    for (int8_t c = 11; c >= 0; c--) {
      // 16 bits per channel, send MSB first
      SPI.transfer(channelStates[n * 12 + c].pwm >> 8);
      SPI.transfer(channelStates[n * 12 + c].pwm);
    }
  }

  delayMicroseconds(200);
  SPI.endTransaction();
}

// Set all channels to same pwm and ramp setting. 
void setAllChannels(long pwm, int ramp) {
  for(int channel = 0; channel < numChannels; channel++){
    channelStates[channel].pwm = pwm;
    channelStates[channel].ramp = ramp;  
  }
}

// Fade lamps according to setting for each channel in the ramp element of the channel's state.
void fadeLamps() {
  for(int iter = 0; iter < 258; iter++) {
    tlcWrite();
    for(int channel = 0; channel < numChannels; channel++){
        channelStates[channel].pwm = calculateNewPwm(channelStates[channel].pwm,channelStates[channel].ramp); 
    }
    delay(10);
  }
}

// Identify a single channel to be faded up, mark others to be faded down. 
void highlightLamp(uint32_t highlitChannel){
  for(int channel = 0; channel < numChannels; channel++){
    channelStates[channel].ramp = RAMP_DOWN;        
  }
  channelStates[highlitChannel].ramp = RAMP_UP;  
}

// Calculate the new pwm value based on current pwm and ramp values.
long calculateNewPwm(long pwm, int ramp){
  long newPwm = pwm + ramp;
  if(newPwm < MIN_PWM) {
    newPwm = MIN_PWM;
  }
  else if(newPwm > MAX_PWM){
    newPwm = MAX_PWM;    
  }
  return newPwm;
}

// Test that a channel number is within the range of available channels.
bool channelInRange(int chan) {
  return (chan > -1 && chan < numChannels);
}

// Print current channel states
void printChannelStates() {
  for(int channel = 0; channel < numChannels; channel++){
    Serial.print(channel);
    Serial.print("   ");
    Serial.print(channelStates[channel].pwm);
    Serial.print("   ");
    Serial.println(channelStates[channel].ramp); 
  }
}


// Funtions for I2C PICO - PICO comms
void initWire() {
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
}

// Read from the slave and print out
char requestLastPadTouched() { 
  char response = noPad;
   
  Wire.requestFrom(0x30,1);
  while (Wire.available()) {
    response = (char) Wire.read();
  }
  return response;
}

// Functions for DF1201
void initDF1201(){
  // Start SS and initialise DF12015 object
  DF1201SSerial.begin(115200);
  delay(1000);
  while(!DF1201S.begin(DF1201SSerial)){
    Serial.println("Init failed, please check the wire connection!");
    delay(10000);
  }
  // Set volume to avoid 'music' announcement
  DF1201S.setVol(0);
  // Set Mode
  DF1201S.switchFunction(DF1201S.MUSIC);
  // Wait for the end of the prompt tone 
  delay(2000);
  DF1201S.setVol(VOLUME);
  // Set playback mode to "repeat all" 
  DF1201S.setPlayMode(DF1201S.SINGLE);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  while(!Serial){}
  // Start PICO - PICO comms
  initWire();
  // Start the DF1201 sound player
  initDF1201();
  // Play the sound
  DF1201S.playFileNum(TRACK);
  digitalWrite(LED_BUILTIN,HIGH);
}

void loop() {
  uint32_t channelIn = 99;

  // Read from the touch reader PICO
  Wire.requestFrom(0x30,1);
  while (Wire.available()) {
    channelIn = (char) Wire.read() - 'A';
  }
  
  if(channelInRange(channelIn)) {
    Serial.print("recv: ");
    Serial.println(channelIn);
    DF1201S.playFileNum(TRACK);
    rp2040.fifo.push_nb(channelIn);
  }

  /* Read from console
  while(Serial.available()) {       
    channelIn = Serial.read() - 'A';
    if(channelInRange(channelIn)) {
      DF1201S.playFileNum(TRACK);
      rp2040.fifo.push_nb(channelIn);
    } 
  }
  */
  delay(10);
}

void setup1() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  setupSPI();
  delay(100);
  setAllChannels(MIN_PWM, RAMP_UP);
  fadeLamps();
}

void loop1() {
  static long lastHighlightTS = 0;

  if(rp2040.fifo.available() > 0){
    uint32_t highlight = rp2040.fifo.pop();
    highlightLamp(highlight);
    fadeLamps();
    lastHighlightTS = millis();
  }
  else if(lastHighlightTS > 0 && millis() > (lastHighlightTS + HIGHLIGHT_TIMEOUT)) {
      setAllChannels(MIN_PWM, RAMP_UP);
      fadeLamps();
      lastHighlightTS = 0;
  }
  delay(20);
}
