#include <serial-readline.h>

// default 2 pin setup
// make sure to change this between devices
const char* DEVICE_ID = "ANALOG_2_01";
// #define FIVE_PIN_SETUP
#define TWO_PIN_SETUP

// five pin setup
//const char* DEVICE_ID = "ANALOG_5_01";

// set the board type
#define BOARD_TYPE_NANO

#ifdef BOARD_TYPE_NANO
// Arduino Uno has 6, Arduino Nano Every has 8
const int NUM_PINS = 8;
// Nano every pinout
int pins[NUM_PINS] = {A0, A1, A2, A3, A4, A5, A6, A7};

// 2 pin setup
#ifdef TWO_PIN_SETUP
  bool activePins[NUM_PINS] = {true, true, false, false, false, false, false, false};
#endif

// five pin setup
#ifdef FIVE_PIN_SETUP
  bool activePins[NUM_PINS] = {false, true, true, true, true, true, false, false};
#endif
#endif

#ifdef BOARD_TYPE_PICO
// Raspberry Pi Pico has 3 (ADC0, ADC1, ADC2)
const int NUM_PINS = 3;
// Pico pinout
int pins[NUM_PINS] = {26, 27, 28};
bool activePins[NUM_PINS] = {true, true, false};
#endif

// the current value of the pins
uint16_t vals[NUM_PINS];

// multi-sampling the analog input.
const uint8_t NUM_SAMPLES = 6;
uint16_t sampleVals[NUM_PINS * NUM_SAMPLES];
// the last sent values
uint16_t sentVals[NUM_PINS];

uint8_t threshold = 3; // threshold for sending data

// delay time between each frame. 60FPS should really be enough, right?
const uint8_t FRAMERATE = 17; // 17ms -> ~60FPS

// make sure we can read strings
void received(char*);
SerialLineReader reader(Serial, received);

void setup() {
  Serial.begin(9600);           //  setup serial
  Serial.println("AnalogReady");
  pinMode(LED_BUILTIN, OUTPUT);

  // blink LED to show we're ready
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);

  // set some output pins to 5V so we can use them
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);

  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);
  digitalWrite(10, HIGH);
  digitalWrite(11, HIGH);
}

void received(char *line) {
  String s = String(line);

  // @TODO: make this dependent on the number of pinouts
  if(s.length() == 16) {
    if(s.substring(0, 7) == "SetPins") {
      Serial.println("U:PinOuts");
      // get bool values
      for(uint8_t i = 0; i < NUM_PINS; i++){
        String inputString = s.substring(8 + i, 9 + i);
        uint16_t val = inputString.toInt();
        //Serial.println(String(i) + ":" + String(val));
        // set active
        activePins[i] = val == 1 ? true : false;
      }
    } else {
      Serial.println("E:SetPins Format: SetPins:XXXXXXXX");
      return;
    }
  } else if(s.length() == 6){
    if(s.substring(0, 4) == "SetT") {
      Serial.println("U:Treshold");
      String inputString = s.substring(5, 6);
      uint16_t val = inputString.toInt();
      threshold = val;
    } else {
      Serial.println("E:SetT Format: SetT:X");      
    }
  } else if(s.length() == 5) {
    if(s.substring(0, 5) == "GetID") {
      Serial.print("I:ID:");
      Serial.println(DEVICE_ID);
    }
  } else if(s.length() == 7) {
    if(s.substring(0, 7) == "GetPins") {
      Serial.print("I:Pins:");
      String pinOut = "";
      for(uint8_t i = 0; i < NUM_PINS; i++){
        
        //Serial.println(String(i) + ":" + String(val));
        // set active
        pinOut += activePins[i] == true ? "1" : "0";
      }
      Serial.println(pinOut);
    }
    if(s.substring(0, 7) == "GetVals") {
      printVals();
    }
  }
}

void printVals() {
  for(int i = 0; i < NUM_PINS; i++){
  // if the pin is not active, dont update or send anything
    if(activePins[i] != true) continue;
    int curVal = analogRead(pins[i]);  // read the input pin
    Serial.println("OUT:" + String(i) + ":" + String(vals[i]));
  }
}

void sampleOnce(uint8_t index) {
  for(uint8_t i = 0; i < NUM_PINS; i++){
    // if the pin is not active, dont update or send anything
    if(activePins[i] != true) continue;
    else {
      uint16_t curVal = analogRead(pins[i]);  // read the input pin
      sampleVals[i + index * NUM_PINS] = curVal;
    }
  }
}

void sample() {
  for(uint8_t i = 0; i < NUM_SAMPLES; i++) {
    sampleOnce(i);
    delay(1);
  }
  // calculate value from samples
  for(uint8_t i = 0; i < NUM_PINS; i++) {
    vals[i] = 0;
    for(uint8_t j = 0; j < NUM_SAMPLES; j++) {
      vals[i] += sampleVals[j * NUM_PINS + i];
    }
    vals[i] /= NUM_SAMPLES;
  }
}

void loop() {
  reader.poll();

  delay(FRAMERATE - NUM_SAMPLES);
  sample();

  for(uint8_t i = 0; i < NUM_PINS; i++){
    // if the pin is not active, dont update or send anything
    if(activePins[i] != true) continue;

    if(abs(sentVals[i] - vals[i]) > threshold) {
      // send again
      Serial.println("OUT:" + String(i) + ":" + String(vals[i]));
      sentVals[i] = vals[i];
    }
  }
}