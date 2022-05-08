#include <FastLED.h>

#define LED_PIN     A0
#define NUM_LEDS    240
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

DEFINE_GRADIENT_PALETTE (heatmap_gp) {
    0,  0,  0,  0,  //black
  128,255,  0,  0,  //red
  200,255,255,  0,  //yellow
  255,255,255,255   //full white
};

DEFINE_GRADIENT_PALETTE (greenblue_gp) {
    0,  0,255,245,  //
   46,  0, 21,255,  //
  179, 12,250,  0,  //
  255,  0,255,245   //
};

// Gradient palette "Sunset_Real_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Sunset_Real.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.
DEFINE_GRADIENT_PALETTE( Sunset_Real_gp ) {
    0, 120,  0,  0,
   22, 179, 22,  0,
   51, 255,104,  0,
   85, 167, 22, 18,
  135, 100,  0,103,
  198,  16,  0,130,
  255,   0,  0,160};

CRGBPalette16 currentPalette;
TBlendType    currentBlending;
uint8_t paletteIndex = 0;
uint8_t colorIndex[NUM_LEDS];

void setup() {
  FastLED.clear();
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
    
  currentBlending = LINEARBLEND; //NOBLEND;

//THIRD TEST
// Fill the colorIndex array with random numbers
//  for (int i=0; i<NUM_LEDS; i++) {
//    colorIndex[i] = random8();
//  }

  FastLED.clear();
}

void loop() {
// comment out one ot the tests below

//FIRST TEST
  // 
  currentPalette = Sunset_Real_gp; //heatmap_gp; // choose palette
  fill_palette(leds, NUM_LEDS, paletteIndex, 255/NUM_LEDS, currentPalette, 255, currentBlending);
  EVERY_N_MILLISECONDS(50) {
    paletteIndex++;
  }

//SECOND TEST
//  currentPalette = greenblue_gp; //ForestColors_p;
//  EVERY_N_MILLISECONDS(1) {
//    // Switch on an LED at random, choosing a random color from a palette
//    uint8_t r=random8(0, NUM_LEDS - 1);
//    leds[r] = ColorFromPalette(currentPalette, random8(), 255, currentBlending);
//  }
//  // Fade all LEDs down by 1 in brightness each time this is called
//  fadeToBlackBy(leds, NUM_LEDS, 1);

//THIRD TEST
//  currentPalette = Sunset_Real_gp;
//  // Color each pixel from the palette using the index from colorIndex[]
//  for (int i=0; i<NUM_LEDS; i++) {
//    leds[i] = ColorFromPalette(currentPalette, colorIndex[i]);
//  }
//  EVERY_N_MILLISECONDS(1) {
//    for (int i=0; i<NUM_LEDS; i++) {
//      colorIndex[i]++;
//    }
//  }

  FastLED.show();
}
