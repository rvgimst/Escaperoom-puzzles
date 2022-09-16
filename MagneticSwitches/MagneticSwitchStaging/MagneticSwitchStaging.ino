const byte numMagSwitchPins = 2;
const byte numLEDs = 2;
const byte magSwitchPins[numMagSwitchPins] = { 2, 3 }; //, 3, 4, 5... };
const byte leds[numLEDs] = { 8, 9 };

void setup() {
  // Start a serial connection
  Serial.begin(9600);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);
  
  // Init magnetic sensor pins
  for (int i=0; i < numMagSwitchPins; i++) {
    pinMode(magSwitchPins[i], INPUT_PULLUP);
  }

  // Init LEDs
  for (int i=0; i < numLEDs; i++) {
    pinMode(leds[i], OUTPUT);
  }
}

void loop() {
  //
  for (int i=0; i < numMagSwitchPins; i++) {
    // every mag switch corresponds to 1 LED
    if (digitalRead(magSwitchPins[i])) {
      // LED off
      digitalWrite(leds[i], LOW);
    } else {
      // LED on
      digitalWrite(leds[i], HIGH);
    }
  }
}
