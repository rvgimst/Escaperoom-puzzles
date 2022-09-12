#include<FastLED.h>

//////////////////////
// FastLED variables
//////////////////////
#define NUM_FLEDS 60 // best to have multiple of 12
#define DATA_PIN A0
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS_HI 60
#define BRIGHTNESS_LO 20
CRGB fastleds[NUM_FLEDS];

// for correct distribution of red/blue colors around the rings
#define RINGDIVIDER 12;
byte numLEDsInPattern = NUM_FLEDS/RINGDIVIDER;

// for fade-in effect when mirror in startup phase
bool fadeIn = true;
byte dynamicBrightnessLevel = 0;

// for smooth motion of the ring pattern
uint8_t speed = 2; // used for stepping to next pattern position (between 1-16)
int patternPosition = 0; // virtual position of the pattern (16 positions == 1 pixel)
int blendrate = 0;
int blenddirection = 1;

//////////////////////////////
// Magnetic Switch variables
//////////////////////////////
const byte numProps = 3;
const byte numMagSwitchPins = 2; // number of switches per prop
const byte MagSwitchPins[numProps][numMagSwitchPins] = { {5, 6}, {7, 8}, {9, 9} }; // We use 1 switch for the fuse
#define PROP_P1_IDX 0
#define PROP_P2_IDX 1
#define PROP_FUSE_IDX 2

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

bool isPowerSourceActivated(byte src) {
  // 'src' indicates the power source to be tested
  // Assume power source is activated until detected otherwise
  bool powerActivated = true;
  byte id = (src == 0 ? PROP_P1_IDX : PROP_P2_IDX); // make sure only valid values are triggered in arrays
  
  for (int i=0; i < numMagSwitchPins; i++) { 
    // check the Power Source
    powerActivated &= !digitalRead(MagSwitchPins[id][i]);
  }

  // For testing with LEDs
  if (powerActivated) {
    // LED on
    //Serial.println((String)"P" + id + " activated...");
    digitalWrite(leds[id], HIGH);
  } else {
    // LED off
    //Serial.println((String)"P" + id + " DEactivated...");
    digitalWrite(leds[id], LOW);
  }

  return powerActivated;
}

bool isFuseActivated() {
  // Assume fuse activated until detected otherwise
  bool fuseActivated = true;
  for (int i=0; i < numMagSwitchPins; i++) { 
    // check the Fuse
    fuseActivated &= !digitalRead(MagSwitchPins[PROP_FUSE_IDX][i]);
  }
  return fuseActivated;
}

void StartPulsatingFastLEDs(byte maxBrightness) {
  // own pattern of LEDs: red every 5 LEDS, others blue
  for (int i=0; i<NUM_FLEDS; i++) {
    if (i % numLEDsInPattern == 0) {
      fastleds[i] = CRGB::Red;
    } else {
      fastleds[i] = CRGB::Blue;
    }
  }

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

void StartRotatingFastLEDS() {
  EVERY_N_MILLISECONDS(10) {
    // advance pattern 1 step. (16 steps corresponds to 1 complete pixel shift)
    patternPosition += speed;
    // Extract the 'fractional' part of the position - that is, what proportion are we into the front-most pixel (in 1/16ths)
    uint8_t frac = patternPosition % 16;
    for (int i=0; i<NUM_FLEDS; i++) {
      if (patternPosition == NUM_FLEDS*16) {
        patternPosition = 0;
      }
      // calculate shifted index, looping around
      byte ledidx = (i + patternPosition/16) % NUM_FLEDS;
       
      if (i % numLEDsInPattern == 0) { // red pixel turning blue
        fastleds[ledidx] = blend(CRGB::Red, CRGB::Blue, frac*16);
      } else if (i % numLEDsInPattern == 1){ // blue pixel in front of red turning red
        fastleds[ledidx] = blend(CRGB::Blue, CRGB::Red, frac*16);
      } else { // other pixels are blue
        fastleds[ledidx] = CRGB::Blue;
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
  //Serial.println((String)"STATE=" + state);
  //StartPulsatingFastLEDs();

  switch (state) {
    case 0:
      ResetFastLEDs(); // Reset LED strip in state0
      if ((isPowerSourceActivated(PROP_P1_IDX) && !isPowerSourceActivated(PROP_P2_IDX)) ||
          (!isPowerSourceActivated(PROP_P1_IDX) && isPowerSourceActivated(PROP_P2_IDX))) {
        state = 1;
        Serial.println("P1 XOR P2 ACTIVATED. State 0 -> State 1");
      }
      break;
    case 1:
      // PLAY DIM PULSATING LEDs
      StartPulsatingFastLEDs(BRIGHTNESS_LO);
      if (isPowerSourceActivated(PROP_P1_IDX) && isPowerSourceActivated(PROP_P2_IDX)) {
        state = 2;
        Serial.println("P1 AND P2 ACTIVATED. State 1 -> State 2");
      }
      else if (!isPowerSourceActivated(PROP_P1_IDX) && !isPowerSourceActivated(PROP_P2_IDX)) {
        state = 0;
        onetime = true;
        Serial.println("NO POWER ACTIVE. State 1 -> State 0");      
      }
      break;
    case 2:
      // PLAY BRIGHT PULSATING LEDs
      StartPulsatingFastLEDs(BRIGHTNESS_HI);
      // PLAY SOUND 1 (ONCE) - TBD
      
      if (isPowerSourceActivated(PROP_P1_IDX) && isPowerSourceActivated(PROP_P2_IDX)) {
        if (isFuseActivated()) {
          digitalWrite(leds[PROP_FUSE_IDX], HIGH);
          state = 3;
          onetime = true;
          Serial.println("in STATE 2, FUSE activated. State 2 -> State 3");
          // PLAY SOUND 2 // ONCE
          // RELEASE DOOR // ONCE
        } else {
          digitalWrite(leds[PROP_FUSE_IDX], LOW);
          if (onetime) {
            Serial.println("in STATE 1, FUSE INACTIVE");
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
      StartRotatingFastLEDS();
      break;
  }
}
