#include<FastLED.h>

// FastLED variables
#define NUM_FLEDS 180
#define DATA_PIN A0
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS 60
CRGB fastleds[NUM_FLEDS];

#define RINGDIVIDER 24;
byte numLEDsInPattern = NUM_FLEDS/RINGDIVIDER;
bool fadeIn = true;
byte dynamicBrightnessLevel = 0;
int patternPosition = 0; //for simple shifting of LEDs
int blendrate = 0;
int blenddirection = 1;

void setup() {
  // Start a serial connection
  Serial.begin(9600);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);

  // Init FastLED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(fastleds, NUM_FLEDS);
  FastLED.setBrightness(BRIGHTNESS); // set master brightness control
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  FastLED.show();
}

void StartPulsatingFastLEDs() {
  // own pattern of LEDs: red every 5 LEDS, others blue
  for (int i=0; i<NUM_FLEDS; i++) {
    if (i % numLEDsInPattern == 0) {
      fastleds[i] = CRGB::Green;
    } else {
      fastleds[i] = CRGB::Green;
    }
  }

  if (fadeIn) { // first we want to fade in the LED strip
    EVERY_N_MILLISECONDS(40) {
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

uint8_t speed = 3; // between 1-16

void StartRotatingFastLEDS0() {
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

void StartRotatingFastLEDS1() {
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
       
      if (i % numLEDsInPattern <= 1) { // red pixel turning blue
        fastleds[ledidx] = blend(CRGB::Red, CRGB::Blue, frac*16);
      } else if (i % numLEDsInPattern == 2){ // blue pixel in front of red turning red
        fastleds[ledidx] = blend(CRGB::Blue, CRGB::Red, frac*16);
      } else { // other pixels are blue
        fastleds[ledidx] = CRGB::Blue;
      }
    }
    FastLED.show();
  }
}

byte pos[10];
byte hue = 0;

// using multiple offset sawtooth waves to create running LED effect
void StartRotatingFastLEDS2(bool dohue=true) {
  for (byte i=0; i<10; i++) {
    pos[i] = map(beat8(20, i*300), 0, 255, 0, NUM_FLEDS - 1);
    if (dohue) {
      fastleds[pos[i]] = CHSV(hue, 200, 255);
    } else {
      fastleds[pos[i]] = CRGB::Green;
    }
  }
  fadeToBlackBy(fastleds, NUM_FLEDS, 16);

  EVERY_N_MILLISECONDS(30) {
    hue++;
    if (hue == 256) hue = 0;
  }
  
  FastLED.show();
}

// change brightness using fixed pattern
// Num LEDs in pattern = NUM_FLEDS/RINGDIVIDER
// first 1/3 of pixels in pattern: bright
// last 2/3 of pixels in pattern: dim
byte d = numLEDsInPattern / 3; // divider from bright to dim LEDs in pattern
#define DIM_VAL 50
#define BRI_VAL 200

void StartRotatingFastLEDS3() {
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
       
      if (i % numLEDsInPattern == 0) { // bright pixel in front of dim turning dim
        fastleds[ledidx] = blend(CHSV(HUE_BLUE, 255, BRI_VAL), CHSV(HUE_GREEN, 255, DIM_VAL), frac*16);
      } else if (i % numLEDsInPattern == d){ // dim pixel in front of bright turning bright
        fastleds[ledidx] = blend(CHSV(HUE_GREEN, 255, DIM_VAL), CHSV(HUE_BLUE, 255, BRI_VAL), frac*16);
      } else if (i % numLEDsInPattern < d) { // bright pixels 
        fastleds[ledidx] = CHSV(HUE_BLUE, 255, BRI_VAL);
      } else { // dim pixels
        fastleds[ledidx] = CHSV(HUE_GREEN, 255, DIM_VAL);
      }
    }
    FastLED.show();
  }
}

void StartColorBlend() {
  //blending 2 colors. To test effect when moving LEDs
  CRGB col1 = CRGB::Blue;
  CRGB col2 = CRGB::Red;
  CRGB blendcol;

// to test how col1 and col2 look like
//  fill_solid(fastleds, NUM_FLEDS, col1);
//  FastLED.show();
//  delay(5000);
//
//  fill_solid(fastleds, NUM_FLEDS, col2);
//  FastLED.show();
//  delay(5000);

  // CRGB  blend (const CRGB &p1, const CRGB &p2, fract8 amountOfP2)
  // void  fill_solid (struct CRGB *leds, int numToFill, const struct CRGB &color)
  //    fill_solid - fill a range of LEDs with a solid color Example: fill_solid( leds, NUM_LEDS, CRGB(50,0,200));
  EVERY_N_MILLISECONDS(10) {
    if (blendrate > 0) {
      blendcol = blend(col1, col2, blendrate);
    } else {
      blendcol = blend(col2, col1, -blendrate);
    }
    fill_solid(fastleds, NUM_FLEDS, blendcol);

    if (blendrate == 255) {
      blenddirection = -1;
    }
    if (blendrate == -255) {
      blenddirection = 1;
    }
    blendrate = blendrate + blenddirection;
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

#define NUM_STATES 7
byte state = 0;

void loop() {
  switch (state) {
    case 0:
      StartPulsatingFastLEDs();
      break;
    case 1:
      StartColorBlend();
      break;
    case 2:
      StartRotatingFastLEDS0();
      break;
    case 3:
      StartRotatingFastLEDS1();
      break;
    case 4:
      StartRotatingFastLEDS2(false);
      break;
    case 5:
      StartRotatingFastLEDS2(true);
      break;
    case 6:
      StartRotatingFastLEDS3();
      break;
  }
  
  EVERY_N_SECONDS(15) {
    state++;
    if (state == NUM_STATES) state = 0;
  }
}
