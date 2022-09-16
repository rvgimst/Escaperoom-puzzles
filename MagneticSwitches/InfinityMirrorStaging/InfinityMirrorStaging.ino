#include<FastLED.h>

// FastLED variables
#define NUM_FLEDS 60
#define DATA_PIN A0
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS 60
CRGB fastleds[NUM_FLEDS];

#define RINGDIVIDER 12;
byte numLEDsInPattern = NUM_FLEDS/RINGDIVIDER;
bool fadeIn = true;
byte dynamicBrightnessLevel = 0;
int patternPosition = 0; //for simple shifting of LEDs
int blendrate = 0;
int blenddirection = 1;

byte state = 1;

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

uint8_t speed = 2; // between 1-16

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

void loop() {
  switch (state) {
    case 0:
      StartPulsatingFastLEDs();
      break;
    case 1:
      StartRotatingFastLEDS();
      break;
    case 2:
      StartColorBlend();
      break;
  }
  
//  EVERY_N_SECONDS(6) {
//    state = 1;
//  }
}
