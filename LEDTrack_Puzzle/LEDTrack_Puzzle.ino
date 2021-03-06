/**
 * LED track switch puzzle
 * Copyright (c) 2020 Alastair Aitchison, Playful Technology
 *
 * Strips of WS2812 LEDs appear to be laid out on multiple segments of a branching track.
 * LED particles are spawned at regular intervals at one end and advance down the track.
 * There are toggle switches placed at each point where the track branches, and by pushing a
 * switch one way or the other, players can select which branch the particles take (like on a railway).
 *
 * Particles come in several different types - each with a corresponding colour and length.
 * Players must guide particles down the tracks to the correct end points. Once every end point has received
 * three particles of the correct colour, the puzzle is solved (which can activate a relay etc.)
 *
 * The following illustrates the way in which the LEDs are laid out. Numbers in [brackets] are the sequential position
 * of the LED on the strip, which is how they are addressed in the leds array. The scale along the top is the
 * "distance travelled" along the track to reach that point.
 *
 * ==== LAYOUT ESCAPE FROM THE PAST, MD, US ====
 * NOTE: directions on junctions (left/right) below correspond to the physical puzzle when watched from the back of the board
 * NOTE2: 5 LEDs indicated with [*] are redundant LEDs that take no part in/are excluded from the puzzle
 * 
 * Distance Travelled
 *  0   1   2   3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29   30   31   32   33   34   35   36   37
 *                                                                                                 (Track 4) RED
 *                            [66] [67] [68*][69] [70] [71] [72] [73] [74] [75] [76] [77] [78] [79*][80] [81] [82]
 *                          /
 *                  [47] [48] (Switch 2)
 *                 /        \                                                                      (Track 3) YELLOW
 *                /           [49] [50] [51] [52] [53] [54] [55] [56] [57] [58] [59*][60] [61] [62] [63] [64] [65*]
 * [0] [1] [2] [3] (Switch 0)                                                                                                                          (Track 2) BLUE
 *               |                                                                        [32] [33] [34] [35] [36] [37] [38] [39] [40] [41] [42] [43] [44] [45] [46*]
 *               |                                                                       /
 *                \                                    [16] [17] [18] [19] [20] [21] [22] (Switch 3)
 *                 \                                  /                                  \                                (Track 1) GREEN
 *                  |                                /                                    [23] [24] [25] [26] [27] [28] [29] [30*][31]
 *                  [4]  [5]  [6]  [7]  [8]  [9]  [10] (Switch 1)                                            
 *                                                    \         (Track 0) WHITE
 *                                                     [11] [12] [13] [14] [15] 
 *
 */

// INCLUDES
// Library for addressable LED strips. Download from https://github.com/FastLED/FastLED/releases
#include <FastLED.h>

// DEFINES
#define LED_PIN A0
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define BRIGHTNESS  64
// The total number of LEDs across all track segments
#define NUM_LEDS 83
// The greatest number of LEDs on any given track down which a particle can travel
#define MAX_NUM_LEDS_PER_TRACK 40
// for palette on solving the puzzle
#define UPDATES_PER_SECOND 30 //100
 
// STRUCTS
// We'll define a structure to keep all the related properties of an LED particle together
struct Particle {
  // What "type" of particle is this? Determines its colour, length, and the track it is meant to end on
  byte type;
  // Note that "position" doesn't correspond to a particular LED - rather, it represents the "distance travelled" by this particle along the track it is on.
  // The advantage of abstracting this from the LED array itself is that it allows us to jump around the set of LEDs, and also allows us to do sub-pixel positioning
  // So "distance travelled" is actually 16x the distance of one LED.
  unsigned int position;
  // How fast does it advance? Must be between 1-16
  int speed;
  // How long is this particle
  unsigned int length;
  // What track is the particle currently travelling down?
  int track;
  // Is this particle currently active?
  bool alive;
};

// CONSTANTS
// How many switch points are there in the track
const byte numSwitchPins = 4;
// The GPIO pins to which switches are attached
const byte switchPins[numSwitchPins] = { 2, 3, 4, 5 };
// The maximum number of particles that will be alive at any one time
const byte maxParticles = 10;
// Number of milliseconds between each particle being spawned
const int _rate = 5000;
// How many different types of particle are there?
const byte numParticleTypes = 5;
// Define a colour associated with each type of particle
const CRGB colours[numParticleTypes] = { CRGB::White, CRGB::Green, CRGB::Blue, CRGB::Yellow, CRGB::Red};
// Specify the order in which LEDs are traversed down each possible track from start to finish. -1 indicates beyond the end of the track
const unsigned int ledTrack[numParticleTypes][MAX_NUM_LEDS_PER_TRACK] = {
  // LHS (ref to front of the board)
  // NOTE: LEDs that were indicated above with an [*] are omitted here
  {0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 31, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 16, 17, 18, 19, 20, 21, 22, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, -1, -1, -1, -1, -1, -1, -1, -1},
  {0,  1,  2,  3, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 60, 61, 62, 63, 64, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {0,  1,  2,  3, 47, 48, 66, 67, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 80, 81, 82, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
  // RHS (ref to front of the board)
};
// Specify which LEDs should be used as a score counter for each track
const unsigned int scoreLEDs[numParticleTypes][3] = {
  {15, 14, 13},
  {31, 29, 28},
  {45, 44, 43},
  {64, 63, 62},
  {82, 81, 80}
};

// GLOBALS
// How many of the correct-coloured particle are currently sitting at the bottom of each track?
int score[numParticleTypes] = {0, 0, 0, 0, 0};
// The array of LEDs
CRGB leds[NUM_LEDS];
// The time at which a particle was last created
unsigned long _lastSpawned = 0;
// Rather than create a new object each time we spawn a particle, we'll create a fixed "pool" of particles
// and re-use them
Particle particlePool[maxParticles];
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

// Initial setup
void setup() {
  // Start a serial connection
  Serial.begin(9600);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);

  // Initialise the LED strip, specifying the type of LEDs and the pin connected to the data line
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // Instantiate a reusable pool of particles
  for(int i=0; i<maxParticles; i++) {
    // Type, position, speed, length, track, alive all set to default values
    particlePool[i] = Particle{0, 0, 0, 0, 0, false};
  }

  // Initialise all the input pins
  for(int i=0; i<numSwitchPins; i++){
    pinMode(switchPins[i], INPUT_PULLUP);
  }

  // Set a random seed by reading the input value of an (unconnected) analog pin
  randomSeed(analogRead(A5));
}

// This function sets the correct LEDs for a particular particle on the track
int setLEDs(Particle p){
  // Convert the colour into a HSV value so that it can be dimmed without changing hue
  CHSV temp = rgb2hsv_approximate(colours[p.type]);
 
  // Extract the 'fractional' part of the position - that is, what proportion are we into the front-most pixel (in 1/16ths)
  uint8_t frac = p.position % 16;
  // Starting at the front and working backwards, work out the brightness of each of the "n" pixels that the particle occupies
  for(int n=0; n<=p.length && n<=p.position/16; n++) {
    if(n==0) {
      // first pixel in the bar
      temp.v = (frac * 16);
    } else if( n == p.length ) {
      // last pixel in the bar
      temp.v = (255 - (frac * 16));
    } else {
      // middle pixels
      temp.v = (255);
    }
    // Two things to watch out for = first that we don't try to go negative and look up an element from the index that doesn't exist
    // And secondly whether we get to the end of a segment and get a -1 value, which *does* exist in the array but isn't actually an LED.
    unsigned int pos = constrain(p.position/16-n, 0, MAX_NUM_LEDS_PER_TRACK*16-1);
    unsigned int LEDtoLight = ledTrack[p.track][pos];
    if(LEDtoLight != -1) {
      // Note that we don't assign a value, but add to it to allow for cases where two particles might overlap etc.
      leds[LEDtoLight] += temp;
    }
  }
}

// Spawn a new particle
void spawnParticle(){
  // Loop over every element in the particle pool
  for(int i=0; i<maxParticles; i++){
    // If this particle is not currently alive
    if(!particlePool[i].alive){
      // Reset it as a new particle
      particlePool[i].position = 0;
      particlePool[i].speed = 1; // between 1-16
      particlePool[i].type = random(0, 5);
      particlePool[i].length = particlePool[i].type + 1;
      particlePool[i].track = 0;
      particlePool[i].alive = true;
      return;
    }
  }
}


// This function is triggered when every track has a maximum score
// Use it to activate a relay/release a maglock etc.
void onSolve() {
  // palette variables for moving lights
  CRGBPalette16 palettes[] = { RainbowColors_p, RainbowStripeColors_p, CloudColors_p,
    PartyColors_p, LavaColors_p, OceanColors_p, ForestColors_p, HeatColors_p };
  int numPalettes = 8;
  int currentPaletteIdx = 0;
  CRGBPalette16 currentPalette = palettes[currentPaletteIdx];
  TBlendType    currentBlending = NOBLEND; //LINEARBLEND;

  // LED variables
  uint8_t brightness = 64;
  uint8_t colorIndex;
  static uint8_t startIndex = 0;
  
  // timing variables for changing palettes (for testing)
  unsigned long currentTime = millis();
  unsigned long lastTime = currentTime;
  int paletteTimeInterval = 20; // [seconds]
    
  while(true) {
    startIndex = startIndex + 1; /* motion speed */
    colorIndex = startIndex;

    // RvG added pattern that flows over tracks consistently from L->R according to track layout
//    for( int i = 0; i < NUM_LEDS; ++i) {
//        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    for (int d=MAX_NUM_LEDS_PER_TRACK-1; d>=0; --d) {
      for (int t=0; t<numParticleTypes; ++t) {
        //int t=2; // for testing individual tracks
        if (ledTrack[t][d] != -1) {
          leds[ledTrack[t][d]] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        }
      }
      colorIndex += 2;
    }

//    RVG: only for testing palettes
//    currentTime = millis();
//    if (currentTime - lastTime > 1000 * paletteTimeInterval) {
//      lastTime = currentTime;
//      currentPaletteIdx++;
//      if (currentPaletteIdx == numPalettes) currentPaletteIdx = 0;// recycle
//      currentPalette = palettes[currentPaletteIdx];
//    }  
    
    // === confetti ===
    // random colored speckles that blink in and fade smoothly
//    fadeToBlackBy( leds, NUM_LEDS, 10);
//    int pos = random16(NUM_LEDS);
//    leds[pos] += CHSV( gHue + random8(64), 200, 255);

    // === juggle ===
    // Eight colored dots, weaving in and out of sync with each other
//    fadeToBlackBy(leds, NUM_LEDS, 20);
//    byte dothue = 0;
//    for(int i=0; i<8; i++) {
//      leds[beatsin16(i+7, 0, NUM_LEDS-1)] |= CHSV(dothue, 200, 255);
//      dothue += 32;
//    }

    // Send the 'leds' array out to the actual LED strip
    FastLED.show();  
    // Insert a delay to keep the framerate modest
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
}


void loop() {
  // For debug use only, print out the state of each toggle switch
  //Serial.print("status switches: ");
  for(int i=0; i<numSwitchPins; i++){
    //Serial.print(digitalRead(switchPins[i]));
    //if(i<numSwitchPins-1) { Serial.print(","); }
  }
  //Serial.println("");
 
  // Clear the LEDs
  FastLED.clear();

  // Get the current elapsed time
  unsigned long currentTime = millis();

  // Has it been too long since the last time we spawned a particle?
  if(currentTime > _lastSpawned + _rate){
    //Serial.println("Time to spawn!");
    spawnParticle();
    _lastSpawned = currentTime;
  }

  // Loop through all the particles
  for(int i = 0; i<maxParticles; i++){
   
    // Only if it's alive!
    if(particlePool[i].alive){
      //Serial.print("random=");
      //Serial.println(random(2));
      // RVG: use the LED index of the switch locations (where the tracks split)
      // If the particle is on one of the switch points, change track as appropriate
      if(particlePool[i].position/16 == 3) {
        particlePool[i].track = (digitalRead(switchPins[0])) ? 0 : 3;
        //particlePool[i].track = (random(2)) ? 0 : 3; // RVG for testing
      }
      // If we took a left (front side) at the first junction then we come across the next switch after in total 10 LEDs
      if(particlePool[i].position/16 == 10 && particlePool[i].track < 3) {
        particlePool[i].track = (digitalRead(switchPins[1])) ? 0 : 1;
        //particlePool[i].track = (random(2)) ? 0 : 1; // RVG for testing
      }
      // If we took a right (front side) at the first junction then we come aross the next switch after 5 LEDs
      if(particlePool[i].position/16 == 5 && particlePool[i].track >= 3) {
        particlePool[i].track = (digitalRead(switchPins[2])) ? 3 : 4;
        //particlePool[i].track = (random(2)) ? 3 : 4; // RVG for testing
      }
      // If we went left, then right, there's a final junction after 17 LEDs
      if(particlePool[i].position/16 == 17 && particlePool[i].track == 1) {
        particlePool[i].track = (digitalRead(switchPins[3])) ? 1 : 2;
        //particlePool[i].track = (random(2)) ? 1 : 2; // RVG for testing
      }

      // Move along the current track
      particlePool[i].position += particlePool[i].speed;

      // If the particle has reached the last LED of its track
      if(ledTrack[particlePool[i].track][particlePool[i].position/16] == -1 || particlePool[i].position > NUM_LEDS*16) {
        // Kill it
        particlePool[i].alive = false;

        // Did it end in the correct track?
        if(particlePool[i].type == particlePool[i].track && score[particlePool[i].track] < 3) {
          // Increase the score for this track
          score[particlePool[i].track]++;
        }
        // The particle ended in the wrong track
        else if(particlePool[i].type != particlePool[i].track && score[particlePool[i].track] > 0) {
          // Reduce score for this track
          // RVG commented out for now - so no degrading of the score
          //score[particlePool[i].track]--;
        }
      }

      // Draw the particle (check if *still* alive after moving!)
      if(particlePool[i].alive) {
        setLEDs(particlePool[i]);
      }
    }
  }

  // Light up the final LEDs in each track corresponding to the corresponding "score"
  byte totalScore = 0;
  for(int i=0; i<5; i++){
    for(int j=0; j<score[i]; j++){
      leds[scoreLEDs[i][j]] += colours[i];
    }
    totalScore += score[i];
  }
  if(totalScore == 15 /*15 RVG changed for testing*/) {
    onSolve();
  }
 
  // Update the LED display with the new data
  FastLED.show();
  FastLED.delay(20);
}
