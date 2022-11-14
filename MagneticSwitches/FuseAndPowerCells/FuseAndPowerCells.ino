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
#define SNDPROP1 1
#define SNDPROP2 2
#define SNDPROP12 3
#define SNDSOLVED 4
// sound pins. These just set digital pins to high/low for a certain short period to trigger soundboard
const byte snd1Pin = 2;
const byte snd2Pin = 3;
const byte sndSolvedPin = 4;
const byte snd12Pin = 12;
const int soundTriggerPeriod = 100; // ms
const int soundDelay = 4500; // ms
bool triggeredSound = false; // to make sure sounds are not triggered continuously

//////////////////////////////
// Relay variables
//////////////////////////////
#define RELAYACTIVATE HIGH
#define RELAYDEACTIVATE LOW
#define RELAY1 0
#define RELAY2 1
const byte relayPins[2] = { 10, 11 }; // pins used for the relays
const int relayDelay = 9000; // ms - approx delay needed to sync with next puzzle

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
//  State2: Play short sound2 once, run pulsating LEDs algorithm with color of power source 2
//   Action: break at least one of the power source2 switches (eg by moving src2 out a bit) => State0
//   Action: power source 1 placed correctly (all switches triggered) => State3
//  State3: Play short sound3 once, run pulsating LEDs algorithm with combined color1&color2
//   Action: [DISABLED] break at least one of the power source1 switches (eg by moving src1 out a bit) => State2
//   Action: [DISABLED] break at least one of the power source2 switches (eg by moving src2 out a bit) => State1
//   Action: fuse placed correctly => State4
//  State4 (end): Play sound4 once, run rotating LEDs algorithm, deactivate electromagnet
byte state = 0; // start state

// Timing variables
bool onetime = true; // to prevent lots of debug output
unsigned long timerStart;

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
    digitalWrite(relayPins[i], RELAYDEACTIVATE);
  }

  // Init audio device
  // Setup the sound pins
  pinMode(snd1Pin, OUTPUT);
  digitalWrite(snd1Pin, HIGH);
  pinMode(snd2Pin, OUTPUT);
  digitalWrite(snd2Pin, HIGH);
  pinMode(snd12Pin, OUTPUT);
  digitalWrite(snd12Pin, HIGH);
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
    case SNDPROP1:
      soundPin = snd1Pin;
      break;
    case SNDPROP2:
      soundPin = snd2Pin;
      break;
    case SNDPROP12:
      soundPin = snd12Pin;
      break;
    case SNDSOLVED:
      soundPin = sndSolvedPin;
      break;
    default: // SNDNONE: set all pins high (sound stops as soon as it has played)
      digitalWrite(snd1Pin, HIGH);
      digitalWrite(snd2Pin, HIGH);
      digitalWrite(snd12Pin, HIGH);
      digitalWrite(sndSolvedPin, HIGH);
      return;
  }

  if (!triggeredSound) {
    Serial.println((String)"triggering sound on pin " + soundPin);
    digitalWrite(soundPin, LOW);
    triggeredSound = true;
    delay(soundTriggerPeriod);
    digitalWrite(soundPin, HIGH);
  }
}

// Set a specific relay to a specific state (ACTIVATE/DEACTIVATE)
void setRelay(byte relayID, int relayState) {
  digitalWrite(relayPins[relayID], relayState);
}

void setRelays(int relayState) {
  // set all relays
  setRelay(RELAY1, relayState);
  setRelay(RELAY2, relayState);
}

void loop() {
  switch (state) {
    case 0:
      triggerSound(SNDNONE); // RESET SOUND (just to make sure)
      setRelays(RELAYDEACTIVATE); // relays back to init state
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
      triggerSound(SNDPROP1); // PLAY SHORT SOUND PROP1
      setRelays(RELAYDEACTIVATE); // relays back to init state

      // PLAY PULSATING LEDs in color Power Source 1
      targetPalette = Blue_gp;
      RunPulsatingFastLEDs();
      if (isPropActivated(PROP_P1_IDX) && isPropActivated(PROP_P2_IDX)) {
        state = 3;
        triggeredSound = false;
        triggerSound(SNDPROP2); // PLAY SHORT SOUND PROP2
        timerStart = millis();
        triggeredSound = false; // for the second sound to be played
        Serial.println("P1 AND P2 ACTIVATED. State 1 -> State 3");
      } else if (!isPropActivated(PROP_P1_IDX)) {
        state = 0;
        triggeredSound = false;
        onetime = true;
        Serial.println("NO POWER ACTIVE. State 1 -> State 0");      
      }
      break;
    case 2:
      triggerSound(SNDPROP2); // PLAY SHORT SOUND PROP2
      setRelays(RELAYDEACTIVATE); // relays back to init state

      // PLAY PULSATING LEDs in color Power Source 2
      targetPalette = Yellow_gp;
      RunPulsatingFastLEDs();
      if (isPropActivated(PROP_P1_IDX) && isPropActivated(PROP_P2_IDX)) {
        state = 3;
        triggeredSound = false;
        triggerSound(SNDPROP1); // PLAY SHORT SOUND PROP1
        timerStart = millis();
        triggeredSound = false; // for the second sound to be played
        Serial.println("P1 AND P2 ACTIVATED. State 2 -> State 3");
      } else if (!isPropActivated(PROP_P2_IDX)) {
        state = 0;
        triggeredSound = false;
        onetime = true;
        Serial.println("NO POWER ACTIVE. State 2 -> State 0");      
      }
      break;
    case 3:
      // delay sound till sound from state 1 or 2 are finished (approximated by soundDelay variable)
      if (millis() - timerStart > soundDelay) {
        triggerSound(SNDPROP12); // PLAY SHORT SOUND BOTH PROPS INSERTED
      }
      setRelay(RELAY2, RELAYACTIVATE); // ACTIVATE relay2

      // PLAY PULSATING LEDs in combined color Power Sources 1+2
      targetPalette = Green_gp;
      RunPulsatingFastLEDs();
// RVG & Fred 11NOV22: decided to not go back to states 0,1,2 once 2 power cells are inserted
//      if (!isPropActivated(PROP_P1_IDX)) {
//        state = 2;
//        triggeredSound = false;
//        Serial.println("P1 DEACTIVATED, P2 still ACTIVE. State 3 -> State 2");
//      } else if (!isPropActivated(PROP_P2_IDX)) {
//        state = 1;
//        triggeredSound = false;
//        Serial.println("P2 DEACTIVATED, P1 still ACTIVE. State 3 -> State 1");
//      } else if (isPropActivated(PROP_FUSE_IDX)) {
//        state = 4;
//        triggeredSound = false;
//        Serial.println("FUSE ACTIVATED. State 3 -> State 4");
//      }
      if (isPropActivated(PROP_FUSE_IDX)) {
        state = 4;
        triggeredSound = false;
        timerStart = millis();
        Serial.println("FUSE ACTIVATED. State 3 -> State 4");
      }
      break;
    case 4:
      triggerSound(SNDSOLVED); // PLAY SOUND ALL PROPS INSERTED
      // delay relay activation till enough time has passed to be in sync with next puzzle
      if (millis() - timerStart > relayDelay) {
        setRelay(RELAY1, RELAYACTIVATE); // ACTIVATE relay1
        setRelay(RELAY2, RELAYDEACTIVATE); // DEACTIVATE relay2 (not needed enymore after solving puzzle
      }

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
