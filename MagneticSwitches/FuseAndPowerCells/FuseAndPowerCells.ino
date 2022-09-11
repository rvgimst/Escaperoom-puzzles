#include<FastLED.h>

// FastLED variables
#define NUM_FLEDS 60
#define DATA_PIN A0
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS 60
CRGB fastleds[NUM_FLEDS];
bool fadeIn = true;
byte dynamicBrightnessLevel = 0;

// Magnetic Switch variables
const byte numP1MagSwitchPins = 2;
const byte numP2MagSwitchPins = 2;
const byte numFuseMagSwitchPins = 2;
const byte power1MagSwitchPins[numP1MagSwitchPins] = { 5, 6 };
const byte power2MagSwitchPins[numP2MagSwitchPins] = { 7, 8 }; // for testing, you can use 1 switch for all triggers
const byte fuseMagSwitchPins[numFuseMagSwitchPins] = { 9, 9 };

// LEDs - for testing
const byte NUM_LEDS = 3;
const byte leds[NUM_LEDS] = { 2, 3, 4 };

// This puzzle consist of 3 props: 2 power sources (prop0 and prop1) and 1 fuse (prop2)
// Each prop needs to be placed in its corresponding receptor/holder.
// Each receptor has a set of magnet switches that all need to be triggered to activate the prop
//
// inputs: magnet switches in each receptor
// outputs: 
//  power sources activated: sound + pulsating LEDs
//  fuse activated: sound + rotating LEDs + opening hatch by... TBD (servo, electromagnet,...?)
// Logics of the puzzle:
//  State0 (start): activate electromagnet for door, everything else off
//   Action: 2 power sources placed correctly (all switches triggered) => State1
//  State1: Play sound1 once, run pulsating LEDs algorithm
//   Action: break at least one of the power source switches (eg by moving one out a bit) => State0
//   Action: fuse placed correctly => State2
//  State2 (end): Play sound2 once, run rotating LEDs algorithm, deactivate electromagnet
byte state = 0; // start state

bool onetime = true; // to prevent lots of debug output

void setup() {
  // Start a serial connection
  Serial.begin(9600);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);

  // Init electromagnet relay
  // TBD
  
  // Init magnetic sensor pins
  for (int i=0; i < numP1MagSwitchPins; i++) {
    pinMode(power1MagSwitchPins[i], INPUT_PULLUP);
  }
  for (int i=0; i < numP2MagSwitchPins; i++) {
    pinMode(power2MagSwitchPins[i], INPUT_PULLUP);
  }
  for (int i=0; i < numFuseMagSwitchPins; i++) {
    pinMode(fuseMagSwitchPins[i], INPUT_PULLUP);
  }

  // Init LEDs
  for (int i=0; i < NUM_LEDS; i++) {
    pinMode(leds[i], OUTPUT);
  }

  // Init FastLED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(fastleds, NUM_FLEDS);
  FastLED.setBrightness(BRIGHTNESS); // set master brightness control
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
}

bool isPowerSourcesActivated() {
  // Assume power sources activated until detected otherwise
  bool power1Activated = true;
  bool power2Activated = true;
  for (int i=0; i < numP1MagSwitchPins; i++) { 
    // check the Power1 Source
    power1Activated &= !digitalRead(power1MagSwitchPins[i]);
  }

  for (int i=0; i < numP2MagSwitchPins; i++) { 
    // check the Power2 Source
    power2Activated &= !digitalRead(power2MagSwitchPins[i]);
  }

  // For testing with LEDs
  if (power1Activated) {
    // LED on
    Serial.println("P1 activated...");
    digitalWrite(leds[0], HIGH);
  } else {
    // LED off
    Serial.println("P1 DEactivated...");
    digitalWrite(leds[0], LOW);
  }
  if (power2Activated) {
    // LED on
    Serial.println("P2 activated...");
    digitalWrite(leds[1], HIGH);
  } else {
    // LED off
    Serial.println("P2 DEactivated...");
    digitalWrite(leds[1], LOW);
  }

  return (power1Activated && power2Activated);
}

bool isFuseActivated() {
  // Assume fuse activated until detected otherwise
  bool fuseActivated = true;
  for (int i=0; i < numFuseMagSwitchPins; i++) { 
    // check the Fuse
    fuseActivated &= !digitalRead(fuseMagSwitchPins[i]);
  }
  return fuseActivated;
}

void StartPulsatingFastLEDs() {
  // own pattern of LEDs: red every 5 LEDS, others blue
  for (int i=0; i<NUM_FLEDS; i++) {
    if (i%5 == 0) {
      fastleds[i] = CRGB::Red;
    } else {
      fastleds[i] = CRGB::Blue;
    }
  }

  if (fadeIn) { // first we want to fade in the LED strip
    EVERY_N_MILLISECONDS(20) {
      FastLED.setBrightness(dynamicBrightnessLevel++);
      FastLED.show();
      if (dynamicBrightnessLevel == BRIGHTNESS) {
        // fade in complete
        fadeIn = false;
        dynamicBrightnessLevel = 0;
      }
    }
  } else {
    // create a sine wave with period of 2 sec (30bpm) to change brightness of the strip
    // and one with 20bpm
    // beatsin8(bpm, minvalue, maxvalue, phase offset, timebase
    uint8_t sinBeat1 = beatsin8(30, 0, 64, 0, 0);
    uint8_t sinBeat2 = beatsin8(60, 0, 64, 0, 0);
  
    FastLED.setBrightness((sinBeat1 + sinBeat2)/2);
    
    EVERY_N_MILLISECONDS(10) {
      Serial.print(sinBeat1 + sinBeat2);
      Serial.print(",");
      Serial.print(sinBeat1);
      Serial.print(",");
      Serial.println(sinBeat2);
    }
    FastLED.show();
  }
}

void ResetFastLEDs() {
  // turn off led strip
  FastLED.setBrightness(0);
  FastLED.show();
  fadeIn = true;
  dynamicBrightnessLevel = 0;
}

void loop() {
  //Serial.println((String)"STATE=" + state);
  //StartPulsatingFastLEDs();

  switch (state) {
    case 0:
      ResetFastLEDs(); // Reset LED strip in state0
      if (isPowerSourcesActivated()) {
        state = 1;
        Serial.println("P1 && P2 activated. State0 -> State1");
      }
      break;
    case 1:
      // PLAY PULSATING LEDs
      StartPulsatingFastLEDs();
      // PLAY SOUND 1 (ONCE) - TBD
      
      if (isPowerSourcesActivated()) {
        if (isFuseActivated()) {
          digitalWrite(leds[2], HIGH);
          state = 2;
          onetime = true;
          Serial.println("in STATE 1, FUSE activated. State1 -> State2");
          // PLAY SOUND 2 // ONCE
          // RELEASE DOOR // ONCE
        } else {
          digitalWrite(leds[2], LOW);
          if (onetime) {
            Serial.println("in STATE 1, FUSE INACTIVE");
            onetime = false;
          }
        }
      } else { // at least one of the power sources is deactivated
        state = 0;
        onetime = true;
        Serial.println("Power DEactivated. State1 -> State0");
      }
      break;
    case 2:
      if (onetime) {
        Serial.println("In STATE2 now...");
        onetime = false;
      }
      // PLAY ROTATING LEDs
      ResetFastLEDs(); // for test, reset
      break;
  }
}
