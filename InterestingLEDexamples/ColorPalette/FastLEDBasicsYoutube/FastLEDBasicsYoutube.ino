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

// Gradient palette "bhw2_xc_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw2/tn/bhw2_xc.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.
DEFINE_GRADIENT_PALETTE( bhw2_xc_gp ) {
    0,   4,  2,  9,
   58,  16,  0, 47,
  122,  24,  0, 16,
  158, 144,  9,  1,
  183, 179, 45,  1,
  219, 220,114,  2,
  255, 234,237,  1};

// Gradient palette "quagga_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/rc/tn/quagga.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.
DEFINE_GRADIENT_PALETTE( quagga_gp ) {
    0,   1,  9, 84,
   40,  42, 24, 72,
   84,   6, 58,  2,
  168,  88,169, 24,
  211,  42, 24, 72,
  255,   1,  9, 84};

// Gradient palette "DEM_screen_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/td/tn/DEM_screen.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.
DEFINE_GRADIENT_PALETTE( DEM_screen_gp ) {
    0,   0, 59,  5,
   31,   3,156,  0,
   63, 227,223, 33,
  127, 227,131,  9,
  191,  67, 32,  2,
  255, 255,255,255};


CRGBPalette16 paletteList[] = {DEM_screen_gp, quagga_gp, bhw2_xc_gp, Sunset_Real_gp, greenblue_gp, heatmap_gp, ForestColors_p};
uint8_t NUM_PALETTES = 7;
uint8_t currentPaletteIndex = 0;
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
uint8_t paletteIndex = 0;
uint8_t colorIndex[NUM_LEDS];
int paletteDuration = 10; // duration in [s] each palette should be shown

void setup() {
  // Start a serial connection
  Serial.begin(9600);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);
  
  FastLED.clear();
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
    
  currentBlending = LINEARBLEND; //NOBLEND;

//THIRD TEST
// Fill the colorIndex array with random numbers
  for (int i=0; i<NUM_LEDS; i++) {
    colorIndex[i] = random8();
  }

  // for looping through different palettes
  currentPaletteIndex = 0;
  currentPalette = paletteList[currentPaletteIndex];

  FastLED.clear();
}

void loop() {
  ChangePalettePeriodically();
// comment out one of the tests below

//FIRST TEST
  // Running palette, decrease ms to increase speed
////  currentPalette = Sunset_Real_gp; //heatmap_gp; // choose palette
//  fill_palette(leds, NUM_LEDS, paletteIndex, 255/NUM_LEDS, currentPalette, 255, currentBlending);
//  EVERY_N_MILLISECONDS(50) {
//    paletteIndex++;
//  }

//SECOND TEST
// set random LEDs to a random palette color with full brightness. Fade the whole LED strip to black every loop
// This causes several LEDs to turn dark, depending on dark colors in the palette
////  currentPalette = greenblue_gp; //ForestColors_p;
//  EVERY_N_MILLISECONDS(1) {
//    // Switch on an LED at random, choosing a random color from a palette
//    uint8_t r=random8(0, NUM_LEDS - 1);
//    leds[r] = ColorFromPalette(currentPalette, random8(), 255, currentBlending);
//  }
//  // Fade all LEDs down by 1 in brightness each time this is called
//  fadeToBlackBy(leds, NUM_LEDS, 1);

//THIRD TEST
// set every LED to a random color of the palette, then shift each individual LED color 1 in the palette
  //currentPalette = greenblue_gp; //Sunset_Real_gp;
  // Color each pixel from the palette using the index from colorIndex[]
  for (int i=0; i<NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex[i]);
  }
  EVERY_N_MILLISECONDS(15) {
    for (int i=0; i<NUM_LEDS; i++) {
      colorIndex[i]++;
    }
  }

  FastLED.show();
}

void ChangePalettePeriodically() {
  static unsigned long lastUpdate = millis();
  unsigned long currentTime = millis();
  unsigned long deltaT = currentTime - lastUpdate;

  if (deltaT >= paletteDuration * 1000) {
    currentPaletteIndex++;

    if (currentPaletteIndex == NUM_PALETTES) {
      currentPaletteIndex = 0; // reset to beginning of palette list
    }
    currentPalette = paletteList[currentPaletteIndex];
    Serial.println(" pi:");
    Serial.println(currentPaletteIndex);
    lastUpdate = currentTime;
  }
}
