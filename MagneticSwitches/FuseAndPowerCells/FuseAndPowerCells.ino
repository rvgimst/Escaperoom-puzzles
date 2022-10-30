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
#define BRIGHTNESS_LO 20 // unused
CRGB fastleds[NUM_FLEDS]; // All LEDs in 1 array

DEFINE_GRADIENT_PALETTE( Yellow_gp ) {
    0, 255,255,  0,
  255, 255,255,  0};

DEFINE_GRADIENT_PALETTE( Blue_gp ) {
    0,   0,  0, 255,
  255,   0,  0, 255};

DEFINE_GRADIENT_PALETTE( Green_gp ) {
    0,  0, 255, 0,
  255,  0, 255, 0
};
   
// black gradient to start with
DEFINE_GRADIENT_PALETTE( Black_gp ) {
    0, 0, 0, 0,
  255, 0, 0, 0};

uint8_t paletteIndex = -1;
CRGBPalette16 currentPalette(Black_gp);
CRGBPalette16 targetPalette(Black_gp);

//////////////////////////////
// Audio variables
//////////////////////////////
// defines for sound types
#define SNDNONE 0
#define SND1PROP 1
#define SND2PROPS 2
#define SNDSOLVED 3
// sound pins. These just set digital pins to high/low for a certain short period to trigger soundboard
const byte snd1Pin = 2;
const byte snd2Pin = 3;
const byte sndSolvedPin = 4;
const int soundTriggerPeriod = 100; // ms

//////////////////////////////
// Relay variables
//////////////////////////////
#define RELAYACTIVATE HIGH  // Fred, change this if needed
#define RELAYDEACTIVATE LOW // Fred, change this if needed
#define RELAY1 0
#define RELAY2 1
const byte relayPins[2] = { 10, 11 }; // pins used for the relays

//////////////////////////////
// Magnetic Switch variables
//////////////////////////////
#define PROP_P1_IDX 0
#define PROP_P2_IDX 1
#define PROP_FUSE_IDX 2
const byte numProps = 3; // 2 power source props, 1 fuse prop
const byte numMagSwitchPins = 2; // number of switches per prop
//const byte MagSwitchPins[numProps][numMagSwitchPins] = { {5, 5}, {7, 7}, {9, 9} }; // For testing, 1 switch for all props
const byte MagSwitchPins[numProps][numMagSwitchPins] = { {5, 6}, {7, 8}, {9, 9} }; // We use 1 switch for the fuse
byte LastPinStatus[numProps][numMagSwitchPins] = { {HIGH, HIGH}, {HIGH, HIGH}, {HIGH, HIGH} };

// LEDs - 1 for each prop (for testing)
//const byte leds[numProps] = { 10, 11, 12 };

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
//   Action: power source 1 placed correctly (all switches triggered) => State1
//   Action: power source 2 placed correctly (all switches triggered) => State2
//  State1: Play short sound1 once, run pulsating LEDs algorithm with color of power source 1
//   Action: break at least one of the power source1 switches (eg by moving src1 out a bit) => State0
//   Action: power source 2 placed correctly (all switches triggered) => State3
//  State2: Play short sound1 once, run pulsating LEDs algorithm with color of power source 2
//   Action: break at least one of the power source2 switches (eg by moving src2 out a bit) => State0
//   Action: power source 1 placed correctly (all switches triggered) => State3
//  State3: Play short sound2 once, run pulsating LEDs algorithm with combined color1&color2
//   Action: break at least one of the power source1 switches (eg by moving src1 out a bit) => State2
//   Action: break at least one of the power source2 switches (eg by moving src2 out a bit) => State1
//   Action: fuse placed correctly => State4
//  State4 (end): Play sound3 continuously, run rotating LEDs algorithm, deactivate electromagnet
byte state = 0; // start state

bool onetime = true; // to prevent lots of debug output

void setup() {
  // Start a serial connection
  Serial.begin(9600);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);

  // Init relay
  // Set the relay pins as output and secure the lock
  // Initialize to ACTIVATE the relay
  // When puzzle changes state, relays are being deactivated
  for (int i=0; i<2; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], RELAYACTIVATE);
  }

  // Init audio device
  // Setup the sound pins
  pinMode(snd1Pin, OUTPUT);
  digitalWrite(snd1Pin, HIGH);
  pinMode(snd2Pin, OUTPUT);
  digitalWrite(snd2Pin, HIGH);
  pinMode(sndSolvedPin, OUTPUT);
  digitalWrite(sndSolvedPin, HIGH);
  
  // Init magnetic sensor pins
  for (int j=0; j < numProps; j++) {           // the props
    for (int i=0; i < numMagSwitchPins; i++) { // the switches for each prop
      pinMode(MagSwitchPins[j][i], INPUT_PULLUP);
    }
  }

  // Init LEDs
//  for (int i=0; i < numProps; i++) {
//    pinMode(leds[i], OUTPUT);
//  }

  // Init FastLED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(fastleds, NUM_FLEDS);
  FastLED.setBrightness(BRIGHTNESS_HI); // set master brightness control
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  fill_palette(fastleds, NUM_FLEDS, paletteIndex, 255/NUM_FLEDS, currentPalette, BRIGHTNESS_HI, NOBLEND);
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
//  if (propActivated) {
//    // LED on
//    //Serial.println((String)"P" + id + " activated...");
//    digitalWrite(leds[id], HIGH);
//  } else {
//    // LED off
//    //Serial.println((String)"P" + id + " DEactivated...");
//    digitalWrite(leds[id], LOW);
//  }

  return propActivated;
}

void RunPulsatingFastLEDs() {
  fill_palette(fastleds, NUM_FLEDS, paletteIndex, 255/NUM_FLEDS, currentPalette, BRIGHTNESS_HI, NOBLEND);

  // create a sine wave with period of 2 sec (30bpm) to change brightness of the strip
  // and one with 20bpm
  // beatsin8(bpm, minvalue, maxvalue, phase offset, timebase
  uint8_t sinBeat1 = beatsin8(30, 0, 64, 0, 0);
  uint8_t sinBeat2 = beatsin8(60, 0, 64, 0, 0);
  
  FastLED.setBrightness((sinBeat1 + sinBeat2)/2);
  FastLED.show();

//    Debug graph output (for serial plotter)
//    EVERY_N_MILLISECONDS(10) {
//      Serial.print(sinBeat1 + sinBeat2);
//      Serial.print(",");
//      Serial.print(sinBeat1);
//      Serial.print(",");
//      Serial.println(sinBeat2);
//    }
}

byte pos[10];
byte hue = 0;

// using multiple offset sawtooth waves to create running LED effect
void RunRotatingFastLEDS(bool dohue=true) {
  for (byte i=0; i<10; i++) { // 10 sawtooth waves, each offset equidistant
    pos[i] = map(beat8(20, i*300), 0, 255, 0, NUM_FLEDS - 1);
    if (dohue) {
      fastleds[pos[i]] = CHSV(hue, 200, 255);
    } else {
      fastleds[pos[i]] = CRGB::Blue;
    }
  }
  fadeToBlackBy(fastleds, NUM_FLEDS, 16);

  EVERY_N_MILLISECONDS(30) {
    hue++;
    if (hue == 256) hue = 0;
  }
  
  FastLED.show();
}

void ResetFastLEDs() {
  // turn off led strip immediately
  FastLED.setBrightness(0);
  FastLED.clear();
  FastLED.show();
}

void triggerSound(int soundType) {
  byte soundPin;
  switch(soundType) {
    case SND1PROP:
      soundPin = snd1Pin;
      break;
    case SND2PROPS:
      soundPin = snd2Pin;
      break;
    case SNDSOLVED:
      soundPin = sndSolvedPin;
      break;
    default: // SNDNONE
      return; // no sound
  }
  
  digitalWrite(soundPin, LOW);
  if (soundType != SNDSOLVED) { // keep signal low (loop sound) when SOLVED
    delay(soundTriggerPeriod);
    digitalWrite(soundPin, HIGH);
  }
}

// Set a specific relay to a specific state (ACTIVATE/DEACTIVATE)
void setRelay(byte relayID, int relayState) {
  digitalWrite(relayPins[relayID], relayState);
}

void activateRelays() {
  // Activate all relays
  setRelay(RELAY1, RELAYACTIVATE);
  setRelay(RELAY2, RELAYACTIVATE);
}

void loop() {
  switch (state) {
    case 0:
      triggerSound(SNDNONE); // RESET SOUND (just to make sure)
      activateRelays(); // ACTIVATE the relays
      ResetFastLEDs(); // Reset LED strip in state0

      if (isPropActivated(PROP_P1_IDX)) {
        state = 1;
        Serial.println("P1 ACTIVATED. State 0 -> State 1");
      } else if (isPropActivated(PROP_P2_IDX)) {
        state = 2;
        Serial.println("P2 ACTIVATED. State 0 -> State 2");
      }
      break;
    case 1:
      triggerSound(SND1PROP); // PLAY SHORT SOUND 1 ONCE
      activateRelays(); // ACTIVATE the relays

      // PLAY PULSATING LEDs in color Power Source 1
      targetPalette = Blue_gp;
      RunPulsatingFastLEDs();
      if (isPropActivated(PROP_P1_IDX) && isPropActivated(PROP_P2_IDX)) {
        state = 3;
        Serial.println("P1 AND P2 ACTIVATED. State 1 -> State 3");
      } else if (!isPropActivated(PROP_P1_IDX)) {
        state = 0;
        onetime = true;
        Serial.println("NO POWER ACTIVE. State 1 -> State 0");      
      }
      break;
    case 2:
      triggerSound(SND1PROP); // PLAY SHORT SOUND 1 ONCE
      activateRelays(); // ACTIVATE the relays

      // PLAY PULSATING LEDs in color Power Source 2
      targetPalette = Yellow_gp;
      RunPulsatingFastLEDs();
      if (isPropActivated(PROP_P1_IDX) && isPropActivated(PROP_P2_IDX)) {
        state = 3;
        Serial.println("P1 AND P2 ACTIVATED. State 2 -> State 3");
      } else if (!isPropActivated(PROP_P2_IDX)) {
        state = 0;
        onetime = true;
        Serial.println("NO POWER ACTIVE. State 2 -> State 0");      
      }
      break;
    case 3:
      triggerSound(SND2PROPS); // PLAY SHORT SOUND 2 ONCE
      setRelay(RELAY1, RELAYDEACTIVATE); // DEACTIVATE relay1

      // PLAY PULSATING LEDs in combined color Power Sources 1+2
      targetPalette = Green_gp;
      RunPulsatingFastLEDs();
      if (!isPropActivated(PROP_P1_IDX)) {
        state = 2;
        Serial.println("P1 DEACTIVATED, P2 still ACTIVE. State 3 -> State 2");
      } else if (!isPropActivated(PROP_P2_IDX)) {
        state = 1;
        Serial.println("P2 DEACTIVATED, P1 still ACTIVE. State 3 -> State 1");
      } else if (isPropActivated(PROP_FUSE_IDX)) {
        state = 4;
        Serial.println("FUSE ACTIVATED. State 3 -> State 4");
      }
      break;
    case 4:
      triggerSound(SNDSOLVED); // PLAY SOUND 3 // CONTINUOUSLY
      setRelay(RELAY2, RELAYDEACTIVATE); // DEACTIVATE relay2

      // PLAY ROTATING LEDs
      RunRotatingFastLEDS();
      if (onetime) {
        Serial.println("In (END)STATE 4 now...");
        onetime = false;
      }
      break;
  }

  nblendPaletteTowardPalette( currentPalette, targetPalette, 80);
}
