#include <serial-readline.h>

const int NUM_PINS = 6;
// the analog pins to read out
int pins[NUM_PINS] = {A0, A1, A2, A3, A4, A5};
// the current value of the pins
int vals[NUM_PINS];
// the last sent values
int sentVals[NUM_PINS];
bool activePins[NUM_PINS] = {true, false, false, false, false, false};

const int threshold = 1; // threshold for sending data

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
}

void received(char *line) {
  String s = String(line);
  if(s.length() != 14) {
    Serial.println("E:SetPins Format: SetPins:XXXXXX");
    return;
  }
  if(s.substring(0, 7) == "SetPins") {
    Serial.println("U:PinOuts");
    // get bool values
    for(int i = 0; i < NUM_PINS; i++){
      String inputString = s.substring(8 + i, 9 + i);
      int val = inputString.toInt();
      //Serial.println(String(i) + ":" + String(val));
      // set active
      activePins[i] = val == 1 ? true : false;
    }
  }
	//Serial.println(line);
}

void loop() {
  reader.poll();
  delay(10);
  for(int i =0; i < NUM_PINS; i++){
    // if the pin is not active, dont update or send anything
    if(activePins[i] != true) continue;

    int curVal = analogRead(pins[i]);  // read the input pin

    if(curVal != vals[i]) {
      vals[i] = curVal;
      if(abs(sentVals[i] - vals[i]) > threshold) {
        // send again
        Serial.println("OUT:" + String(i) + ":" + String(vals[i]));
        sentVals[i] = vals[i];
      }
    }
  }
}