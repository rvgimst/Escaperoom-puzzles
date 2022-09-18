#include<FastLED.h>

//////////////////////
// FastLED variables
//////////////////////
#define NUM_FLEDS_IN   72  // Inner ring - best to have multiple of 12
#define NUM_FLEDS_OUT 108  // Outer ring - best to have multiple of 12
#define NUM_FLEDS NUM_FLEDS_IN + NUM_FLEDS_OUT // we address 1 LED strip for both rings
#define DATA_PIN A0
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS_HI 60
#define BRIGHTNESS_LO 20
CRGB fastleds[NUM_FLEDS]; // All LEDs in 1 array
byte ringStartIdx[2] = { 0, NUM_FLEDS_IN }; // on which pixel does each ring start?
byte ringNumLeds[2]  = { NUM_FLEDS_IN, NUM_FLEDS_OUT }; // num LEDs per ring

// We want a distribution of 12 red/blue colors (clock) around the rings
// eacht ring has a different pattern since the number of LEDs per ring differs
#define RINGDIVIDER 12
byte numLEDsInPattern[2] = { NUM_FLEDS_IN/RINGDIVIDER, NUM_FLEDS_OUT/RINGDIVIDER };

// for mirror LEDs in pulsating phase
bool fadeIn = true; // for fade-in effect
byte dynamicBrightnessLevel = 0; // fade-in start with dark pixels

// for smooth motion of the ring pattern in rotating phase
// To have both rings move parallel they need different speeds, Factor = NUM_FLEDS_OUT/NUM_FLEDS_IN
byte speed[2] = { 2, 3 }; // used for stepping to next pattern position (between 1-16)
                           // negative indicates counter clockwise movement
int patternPosition[2] = { 0, 0 }; // virtual position of the pattern (16 positions == 1 pixel) per ring

//////////////////////////////
// Magnetic Switch variables
//////////////////////////////
#define PROP_P1_IDX 0
#define PROP_P2_IDX 1
#define PROP_FUSE_IDX 2
const byte numProps = 3; // 2 power source props, 1 fuse prop
const byte numMagSwitchPins = 2; // number of switches per prop
const byte MagSwitchPins[numProps][numMagSwitchPins] = { {5, 5}, {7, 7}, {9, 9} }; // We use 1 switch for the fuse
//const byte MagSwitchPins[numProps][numMagSwitchPins] = { {5, 6}, {7, 8}, {9, 9} }; // We use 1 switch for the fuse
byte LastPinStatus[numProps][numMagSwitchPins] = { {HIGH, HIGH}, {HIGH, HIGH}, {HIGH, HIGH} };

// LEDs - 1 for each prop (for testing)
const byte leds[numProps] = { 2, 3, 4 };

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

  // Init audio device
  // TBD
  
  // Init magnetic sensor pins
  for (int j=0; j < numProps; j++) {           // the props
    for (int i=0; i < numMagSwitchPins; i++) { // the switches for each prop
      pinMode(MagSwitchPins[j][i], INPUT_PULLUP);
    }
  }

  // Init LEDs
  for (int i=0; i < numProps; i++) {
    pinMode(leds[i], OUTPUT);
  }

  // Init FastLED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(fastleds, NUM_FLEDS);
  FastLED.setBrightness(BRIGHTNESS_HI); // set master brightness control
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  FastLED.show();
}

bool isPropActivated(byte id) {
  // 'id' indicates the prop to be tested
  // Assume prop is activated until detected otherwise
  bool propActivated = true;
  byte pinStatus;

  for (int i=0; i < numMagSwitchPins; i++) { 
    // check the magnetic switches
    pinStatus = digitalRead(MagSwitchPins[id][i]);
    propActivated &= !pinStatus;
    if (pinStatus != LastPinStatus[id][i]) {
      LastPinStatus[id][i] = pinStatus;
      Serial.println((String)"SwitchPin " + MagSwitchPins[id][i] + " changed to " + pinStatus);
    }
  }

  // For testing with LEDs
  if (propActivated) {
    // LED on
    //Serial.println((String)"P" + id + " activated...");
    digitalWrite(leds[id], HIGH);
  } else {
    // LED off
    //Serial.println((String)"P" + id + " DEactivated...");
    digitalWrite(leds[id], LOW);
  }

  return propActivated;
}

void FillRingPattern(byte rId) {
  // own pattern of LEDs: red every 5 LEDS, others blue
  for (int i = ringStartIdx[rId]; i < ringStartIdx[rId] + ringNumLeds[rId]; i++) {
    if ((i-ringStartIdx[rId]) % numLEDsInPattern[rId] == 0) {
      fastleds[i] = CRGB::Red;
    } else {
      fastleds[i] = CRGB::Blue;
    }
  }
}

void RunPulsatingFastLEDs(byte maxBrightness) {
  // TODO: should only be done once
  FillRingPattern(0); 
  FillRingPattern(1); 

  if (fadeIn) { // first we want to fade in the LED strip
    EVERY_N_MILLISECONDS(20) {
      FastLED.setBrightness(dynamicBrightnessLevel++);
      FastLED.show();
      if (dynamicBrightnessLevel == maxBrightness) {
        // fade in complete
        fadeIn = false;
        dynamicBrightnessLevel = 0;
      }
    }
  } else {
    // create a sine wave with period of 2 sec (30bpm) to change brightness of the strip
    // and one with 20bpm
    // beatsin8(bpm, minvalue, maxvalue, phase offset, timebase
    uint8_t sinBeat1 = beatsin8(30, 0, maxBrightness, 0, 0);
    uint8_t sinBeat2 = beatsin8(60, 0, maxBrightness, 0, 0);
  
    FastLED.setBrightness((sinBeat1 + sinBeat2)/2);
    
//    EVERY_N_MILLISECONDS(10) {
//      Serial.print(sinBeat1 + sinBeat2);
//      Serial.print(",");
//      Serial.print(sinBeat1);
//      Serial.print(",");
//      Serial.println(sinBeat2);
//    }
    FastLED.show();
  }
}

void RunRotatingFastLEDS(byte rId) {
  EVERY_N_MILLISECONDS(10) {
    // advance pattern 1 step. (16 steps corresponds to 1 complete pixel shift)
    patternPosition[rId] += speed[rId];
    // Extract the 'fractional' part of the position - that is, what proportion are we into the front-most pixel (in 1/16ths)
    uint8_t frac = patternPosition[rId] % 16;
    for (int i=0; i<ringNumLeds[rId]; i++) {
      if (patternPosition[rId] == ringNumLeds[rId]*16) {
        patternPosition[rId] = 0;
      }
      // calculate shifted index, looping around
      byte ledidx = (i + patternPosition[rId]/16) % ringNumLeds[rId];
       
      if (i % numLEDsInPattern[rId] == 0) { // red pixel turning blue
        fastleds[ledidx + ringStartIdx[rId]] = blend(CRGB::Red, CRGB::Blue, frac*16);
      } else if (i % numLEDsInPattern[rId] == 1){ // blue pixel in front of red turning red
        fastleds[ledidx + ringStartIdx[rId]] = blend(CRGB::Blue, CRGB::Red, frac*16);
      } else { // other pixels are blue
        fastleds[ledidx + ringStartIdx[rId]] = CRGB::Blue;
      }
    }
    FastLED.show();
  }
}

void ResetFastLEDs() {
  // turn off led strip
  //FastLED.setBrightness(0);
  FastLED.clear();
  FastLED.show();
  fadeIn = true;
  dynamicBrightnessLevel = 0;
}

void loop() {
  switch (state) {
    case 0:
      ResetFastLEDs(); // Reset LED strip in state0
      if ((isPropActivated(PROP_P1_IDX) && !isPropActivated(PROP_P2_IDX)) ||
          (!isPropActivated(PROP_P1_IDX) && isPropActivated(PROP_P2_IDX))) {
        state = 1;
        Serial.println("P1 XOR P2 ACTIVATED. State 0 -> State 1");
      }
      break;
    case 1:
      // PLAY DIM PULSATING LEDs
      RunPulsatingFastLEDs(BRIGHTNESS_LO);
      if (isPropActivated(PROP_P1_IDX) && isPropActivated(PROP_P2_IDX)) {
        state = 2;
        Serial.println("P1 AND P2 ACTIVATED. State 1 -> State 2");
      }
      else if (!isPropActivated(PROP_P1_IDX) && !isPropActivated(PROP_P2_IDX)) {
        state = 0;
        onetime = true;
        Serial.println("NO POWER ACTIVE. State 1 -> State 0");      
      }
      break;
    case 2:
      // PLAY BRIGHT PULSATING LEDs
      RunPulsatingFastLEDs(BRIGHTNESS_HI);
      // PLAY SOUND 1 (ONCE) - TBD
      
      if (isPropActivated(PROP_P1_IDX) && isPropActivated(PROP_P2_IDX)) {
        if (isPropActivated(PROP_FUSE_IDX)) {
          state = 3;
          onetime = true;
          Serial.println("in STATE 2, FUSE activated. State 2 -> State 3");
          // PLAY SOUND 2 // ONCE
          // RELEASE DOOR // ONCE
        } else {
          if (onetime) {
            Serial.println("in STATE 2, FUSE INACTIVE");
            onetime = false;
          }
        }
      } else { // at least one of the power sources is deactivated
        state = 0;
        onetime = true;
        Serial.println("Power DEactivated. State 2 -> State 0");
      }
      break;
    case 3:
      if (onetime) {
        Serial.println("In STATE 3 now...");
        onetime = false;
      }
      // PLAY ROTATING LEDs
      RunRotatingFastLEDS(0);
      RunRotatingFastLEDS(1);
      break;
  }
}
