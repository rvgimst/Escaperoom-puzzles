/**
 * Dual Plasma Ball
 * 
 * This sketch is an escape room puzzle in which players must simultaneously place their hands on two plasma balls,
 * in order to make a linear actuator rise up. When a plasma ball is touched, the current through it rises, and this
 * is detected using an IN219 sensor.
 */

// DEFINES
#define DEBUG

// Audio
// RVG: triggerSound() is the routine that is playing the sound
// For now, every time all balls are touched, the sound is played
// If you want to change this, just call triggerSound() somewhere else in the code
const int soundPin = 2;
const int soundTriggerPeriod = 100; // ms
const bool AudioContinuous = false; // Set this to play continuously or only once

// INCLUDES
// Arduino library for I2C connection
#include <Wire.h>
// Current sensor library. See https://github.com/johngineer/ArduinoINA219
#include "INA219.h"
// Since the INA219 reading can be a bit noisy, we'll smooth out values using a running average of a value
#include "RunningAverage.h"

// CONSTANTS
// You can use up to 16 IN219 devices on the same I2C bus, so long as each is assigned a unique
// address as described in the datasheet http://www.ti.com/lit/ds/symlink/ina219.pdf
const byte numSensors = 2;
const byte relayPins[2] = {A0, A1};

// GLOBALS
// Declare the array of sensors
INA219 sensors[numSensors] = {
  INA219(0x40), // Default I2C address
  INA219(0x41) // I2C address when jumper A1 has been bridged to GND
};
// An array to record the starting reading of each sensor
float baseReading[numSensors];
// RVG: An array that holds reference values in case the readings are too far off
//      (e.g. someone touching the balls at startup)
//      these readings were measured running the puzzle multiple times
const float referenceReading[numSensors] = {1.59, 1.15};
// Array of running averages to smooth readings
RunningAverage averageReadings[numSensors] = {
  RunningAverage(5),
  RunningAverage(5)
};
// The amount by which power measured on each circuit needs to deviate 
// to be interpreted as a touch on the plasma ball
float tolerance = 0.1;

void setup(void) {
  float avg;
  delay(300);

  #ifdef DEBUG
    Serial.begin(115200);
    Serial.println(F("Starting setup..."));
  #endif

  // Setup the sensors
  for(int i=0;i<numSensors;i++){
    // Explicitly clear the running average value
    averageReadings[i].clear();
    // Start the sensor, with default configuration (range of <32V, <2A)
    // If you only need to monitor lower values, you can increase precision by reducing this range
    sensors[i].begin();
  }
  // Sample each sensor several times to get the average base reading
  for(int j=0; j<numSensors; j++){
    for(int k=0; k<5; k++){
      averageReadings[j].addValue(sensors[j].busPower());
      delay(50);
    }    
    avg = averageReadings[j].getAverage();

    #ifdef DEBUG
      Serial.print(F("Sensor "));
      Serial.print(j);
      Serial.print(F(" reads "));
      Serial.println(avg);
    #endif
    // Assign the running average value to the array
    // RVG: but only if the readings are more or less within a reasonable value 
    //     (according to earlier measurements of this puzzle in the escape room)
    //     This is to prevent that the Arduino starts while someone holds one of the balls,
    //     resulting in a false reading and never solving the puzzle.
    if (fabs(avg - referenceReading[j]) > 2*tolerance) { // too far off
      baseReading[j] = referenceReading[j];
      #ifdef DEBUG
        Serial.print(F("Value too far off. BASE initialised with REFERENCE reading "));
      #endif
    } else {
      baseReading[j] = avg;
      #ifdef DEBUG
        Serial.print(F("Value OK. BASE initialised with AVERAGE reading "));
      #endif
    }
    #ifdef DEBUG
      Serial.println(baseReading[j]);
    #endif    
  }

  // Initialise the relay pins 
  for(int i=0;i<2;i++){
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }

  // setup audio
  pinMode(soundPin, OUTPUT);
  digitalWrite(soundPin, HIGH);

  #ifdef DEBUG
    Serial.println(F("Setup complete."));
  #endif

  // Pause to let everything stabilise before running the main program loop
  delay(1000);
}

void triggerSound() {
  if (AudioContinuous) {
    digitalWrite(soundPin, LOW);
  } else {
    digitalWrite(soundPin, LOW);
    delay(soundTriggerPeriod);
    digitalWrite(soundPin, HIGH);
  }
}

bool allBallsBeingTouched() {
  // If all the balls are being touched, then all sensor readings should be outside the
  // tolerance limit from their base (untouched) readings. Put another way, if any sensor
  // reading is *within* the tolerance, then all the balls can't be being touched:
  // RVG: be aware that it is assumed that the avgReading when touched is always higher!
  //      If lower the difference is negative and always < tolerance.
  //      Better use abs() function
  for(int i=0; i<numSensors; i++) {
    if((averageReadings[i].getAverage() - baseReading[i]) < tolerance) {
      return false;
    }
  }
  triggerSound();
  return true;
}

/* Make the linear actuator extend */
void extend() {
  digitalWrite(relayPins[0], LOW);
  digitalWrite(relayPins[1], HIGH);
}

/* Make the linear actuator contract */
void contract() {
  digitalWrite(relayPins[0], HIGH);
  digitalWrite(relayPins[1], LOW);
}

/* Main program loop */
void loop(void)  {
  // Loop over each sensor
  for(int i=0; i<numSensors; i++){
    
    // The bus power is the total power used by the circuit under test, as measured between GND and V- 
    float busPower = sensors[i].busPower();
    
    // Add the current reading to the running array
    averageReadings[i].addValue(busPower);

    #ifdef DEBUG
      Serial.print(averageReadings[i].getAverage());
      // Put comma delimiter between values
      if(i<numSensors-1) {Serial.print(",");}
    #endif
  }
  #ifdef DEBUG
    Serial.println("");
  #endif

  // Activate the actuator based on the inputs
  if(allBallsBeingTouched()) {
    extend();
  }
  else {
    contract();
  }

  // Introduce a slight delay before next polling the sensors
  delay(100);
}
