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

// Gradient palette "bhw1_07_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw1/tn/bhw1_07.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 8 bytes of program space.
DEFINE_GRADIENT_PALETTE( Yellow_gp ) {
    0, 232, 65,  1,
  255, 229,227,  1};

// Gradient palette "bath_112_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/esri/hypsometry/bath/tn/bath_112.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 508 bytes of program space.
DEFINE_GRADIENT_PALETTE( Blue_gp ) {
    0,   0,  3, 34,
   70,   0, 18,123,
  157,   3, 81,255,
  192,  20,124,255,
  216,  51,162,255,
  255, 120,209,255};

// Gradient palette "girlcat_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/rc/tn/girlcat.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.
DEFINE_GRADIENT_PALETTE( Green_gp ) {
    0, 173,229, 51,
   73, 139,199, 45,
  140,  46,176, 37,
  204,   7, 88,  5,
  255,   0, 24,  0};

DEFINE_GRADIENT_PALETTE( BlueSpike_gp ) {
    0,   0,   0,  40,
  127,   0,   0, 255,
  255,   0,   0,  40};

uint8_t paletteIndex = -1;

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

CRGBPalette16 currentPalette(Yellow_gp);
CRGBPalette16 targetPalette(Yellow_gp);
uint8_t whichPalette = 0;

void RunPulsatingFastLEDsUsingPalette() {
  fill_palette(fastleds, NUM_FLEDS, paletteIndex, 255/NUM_FLEDS, currentPalette, BRIGHTNESS, LINEARBLEND);

  // create a sine wave with period of 2 sec (30bpm) to change brightness of the strip
  // and one with 20bpm
  // beatsin8(bpm, minvalue, maxvalue, phase offset, timebase
  uint8_t sinBeat1 = beatsin8(30, 0, 64, 0, 0);
  uint8_t sinBeat2 = beatsin8(60, 0, 64, 0, 0);
  
  FastLED.setBrightness((sinBeat1 + sinBeat2)/2);

  EVERY_N_MILLISECONDS(10) {
//    paletteIndex++;
  }

  FastLED.show();
}

void StartPulsatingFastLEDs() {
  FastLED.setBrightness(BRIGHTNESS);
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
  FastLED.setBrightness(BRIGHTNESS);
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
  FastLED.setBrightness(BRIGHTNESS);
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
  FastLED.setBrightness(BRIGHTNESS);
  for (byte i=0; i<10; i++) {
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

// change brightness using fixed pattern
// Num LEDs in pattern = NUM_FLEDS/RINGDIVIDER
// first 1/3 of pixels in pattern: bright
// last 2/3 of pixels in pattern: dim
byte d = numLEDsInPattern / 3; // divider from bright to dim LEDs in pattern
#define DIM_VAL 50
#define BRI_VAL 200

void StartRotatingFastLEDS3() {
  FastLED.setBrightness(BRIGHTNESS);
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
        fastleds[ledidx] = blend(CHSV(HUE_BLUE, 255, BRI_VAL), CHSV(HUE_BLUE, 255, DIM_VAL), frac*16);
      } else if (i % numLEDsInPattern == d){ // dim pixel in front of bright turning bright
        fastleds[ledidx] = blend(CHSV(HUE_BLUE, 255, DIM_VAL), CHSV(HUE_BLUE, 255, BRI_VAL), frac*16);
      } else if (i % numLEDsInPattern < d) { // bright pixels 
        fastleds[ledidx] = CHSV(HUE_BLUE, 255, BRI_VAL);
      } else { // dim pixels
        fastleds[ledidx] = CHSV(HUE_BLUE, 255, DIM_VAL);
      }
    }
    FastLED.show();
  }
}

CRGBPalette16 BlueSpikePalette(BlueSpike_gp);

// blue 1/3 bright, 2/3 dim moving according to combined sine wave
void StartRotatingFastLEDS4() {
  uint8_t repeat = 12;
  uint8_t wave = NUM_FLEDS/repeat;
  // create a sine wave with period of 2 sec (30bpm) to change brightness of the strip
  // and one with 20bpm
  // beatsin8(bpm, minvalue, maxvalue, phase offset, timebase
  uint8_t sinBeat1 = beatsin8(40, 0, wave, 0, 0);
  uint8_t sinBeat2 = beatsin8(80, 0, wave, 0, 0);

  uint8_t pos = (sinBeat1 + sinBeat2) / 2;
  // map this pos 'repeat' times over the LED strip
  for (int i=0; i<repeat; i++) {
    fastleds[pos+wave*i] = CRGB::Blue;
  }
  fadeToBlackBy(fastleds, NUM_FLEDS, 3);
//  FastLED.setBrightness((sinBeat1 + sinBeat2)/2);
  FastLED.show();

}

void StartColorBlend() {
  //blending 2 colors. To test effect when moving LEDs
  CRGB col1 = CRGB::Blue;
  CRGB col2 = CRGB::Red;
  CRGB blendcol;

  FastLED.setBrightness(BRIGHTNESS);
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

#define NUM_STATES 8
byte state = 7;

void loop0() {
  // this loop tests fade between 2 pulsating effects where colors from 1 blend into the other
  RunPulsatingFastLEDsUsingPalette();

  EVERY_N_SECONDS(10) {
    switch (whichPalette) {
      case 0:
        targetPalette = Blue_gp;
        Serial.println("switching to Blue...");
        break;
      case 1:
        targetPalette = Green_gp;
        Serial.println("switching to Green...");
        break;
      case 2:
        targetPalette = Yellow_gp;
        Serial.println("switching to Yellow...");
        break;
    }
    
    whichPalette++;
    if (whichPalette > 2) whichPalette = 0;
  }
  nblendPaletteTowardPalette( currentPalette, targetPalette, 20);
}

void loop() {
  switch (state) {
    case 0:
      StartPulsatingFastLEDs();
      break;
    case 1:
      StartColorBlend();
      break;
    case 2:
      StartRotatingFastLEDS0(); // 1 red LED in pattern
      break;
    case 3:
      StartRotatingFastLEDS1(); // 2 red LEDs in pattern
      break;
    case 4:
      StartRotatingFastLEDS2(false); // sawtooth waves repeated
      break;
    case 5:
      StartRotatingFastLEDS2(true); // sawtooth waves repeated
      break;
    case 6:
      // change brightness using fixed pattern
      // Num LEDs in pattern = NUM_FLEDS/RINGDIVIDER
      // first 1/3 of pixels in pattern: bright
      // last 2/3 of pixels in pattern: dim
      StartRotatingFastLEDS3(); // 
      break;
    case 7:
      StartRotatingFastLEDS4(); // 
      break;
  }
  
  EVERY_N_SECONDS(15) {
//    ResetFastLEDs();
//    delay(500);
//    state++;
//    if (state == NUM_STATES) state = 0;
  }
}
