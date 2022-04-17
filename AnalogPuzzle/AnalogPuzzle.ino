/**
* Analog Meters Puzzle, Copyright (c) 2021 Playful Technology
* 
* This puzzle requires the player to adjust 4 linear potentiometers to make 4
* analog ammeters all read the correct reading. However, there is not a direct correlation
* between the value of each input and the value of each output ;)
*/

// CONSTANTS
// To write an "analog" output, we need to connect each ammeter to a GPIO pin that supports PWM.
// On an Arduino Nano/UNO, these are pins 3, 5, 6, 9, 10, or 11 
const byte meterPins[] = {11, 10, 9, 3};
// To read an analog input, we need to use a GPIO pin that supports analogRead (i.e. prefixed with "A")
const byte sliderPins[] = {A0, A1, A2, A3};
// LEDs to light when each meter is correct. These are simple digital outputs so can use any spare GPIO pin
const byte ledPins[] = {7, 6, 5, 4};
// This pin will be written LOW when the puzzle is solved
const byte relayPin = 2;
// The values which each meter needs to be set to to solve the puzzle
// These are based on 8-bit scale, where 255 = full deflection to the right, 128 = midpoint
const int targetValues[] = {191, 191, 191, 191};
// How much tolerance either side of target value will we still consider to be correct?
const int toleranceOutput = 10;
// How much threshold on input around zero before input is valid for a solution?
const int thresholdZeroInput = 20;

// GLOBALS
// We'll store all the inputs and outputs in arrays
// 10-bit input values from the ADC have values in the range (0-1023)
int inputValues[4] = {};
// 8-bit output values to pass to AnalogWrite PWM output have values in the range (0-255)
int outputValues[4] = {};
// Has the puzzle been solved?
bool isSolved = false;

// The Arduino map() function produces incorrect distribution for integers. See https://github.com/arduino/Arduino/issues/2466
// So, create a fixed function instead:
long map2(long x, long in_min, long in_max, long out_min, long out_max) {
        return (x - in_min) * (out_max - out_min+1) / (in_max - in_min+1) + out_min;
}

void setup() {
  // Start the serial connection (used for debugging)
  Serial.begin(115200);
  // Print some useful debug output - the filename and compilation time
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);

  // Initialise the pins
  for(int i=0; i<4; i++) {
    pinMode(meterPins[i], OUTPUT);
    pinMode(ledPins[i], OUTPUT);
  }
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
}

void loop() {
  
  // READ INPUTS
  // Loop over every slider
  for(int i=0; i<4; i++){
    // The ADC can retain capacitance from previous read, which causes erroneous results
    // To try to prevent this, we'll take a few "dummy" readings and discard them.
    for(int x=0; x<2; x++) {
      analogRead(sliderPins[i]);
      // And also delay a short time before taking another reading
      delay(5);
    }

    // Now we'll retrieve slider value again, and store in inputValues array
    // RVG: since our sliders are upside-down we invert the inputs to assure 0=low position
    //      and 1023=high position
    inputValues[i] = 1023 - analogRead(sliderPins[i]);
  }

  // CALCULATE LOGIC
  // Apply rules to determine output values from (one or more) inputs
  // You can apply any rules you want - the following are just some examples
  
  // Output is the biggest of two input values
  outputValues[0] = max(inputValues[0], inputValues[2]);

  // Output is the difference of two input values
  outputValues[1] = abs(inputValues[1] - inputValues[3]);

  // Output is the average of two input values
  outputValues[2] = (inputValues[2] + inputValues[3]) / 2;
 
  // Output is the minimum of two input values
  outputValues[3] = min(inputValues[2], inputValues[3]);

  // DISPLAY OUTPUT VALUES AND CHECK AGAINST TARGET
  // Start by assuming that all meters are correct
  // RVG: Added functionality: A meter is only correct if its input with the same index
  //      is not zero (including some threshold)
  //      Rationale: we don't want any solutions where inputs are untouched
  bool allMetersCorrect = true;
  // Loop over each output
  for(int i=0; i<4; i++){
    // Rescale from the 10-bit 0-1023 range of the ADC input to 8-bit 0-255 of PWM output
    //outputValues[i] = map(outputValues[i], 0, 1010, 0, 255);
    outputValues[i] = map2(outputValues[i], 0, 1023, 0, 255);
    outputValues[i] = constrain(outputValues[i], 0, 255);    
    // Write the output to the ammeter
    analogWrite(meterPins[i], outputValues[i]); 

    // If this meter lies outside the allowed tolerance
    if(abs(outputValues[i] - targetValues[i]) > toleranceOutput) {
      // All meters are not correct
      allMetersCorrect = false;
      // Turn this LED off
      digitalWrite(ledPins[i], LOW);
    }
    else {
      // RVG: if the input with same index as output is small (within threshold)
      //      => solution not correct
      if (inputValues[i] - thresholdZeroInput <= 0) {
        // not correct
      allMetersCorrect = false;
        // Turn this LED off
        digitalWrite(ledPins[i], LOW);
      }
      else {
        // Turn the LED on
        digitalWrite(ledPins[i], HIGH);
      }
    }
  }

  // CHECK SOLUTION
  // If we get this far and allMetersCorrect is still true, every value must have been within accepted tolerance
  if(allMetersCorrect && !isSolved) {
    // Solve code here
    Serial.println("Solved!");
    isSolved = true;
    digitalWrite(relayPin, LOW);
  }
 // If the puzzle had been solved, but now the meters are no longer correct
  else if(isSolved && !allMetersCorrect) {
    Serial.println("Unsolved!");
    isSolved = false;
    digitalWrite(relayPin, HIGH);
  }

  // DEBUG
  // If desired, print output to serial monitor for debugging
  for(int i=0; i<4; i++) {
    Serial.print(inputValues[i]);
    Serial.print(",");
    Serial.print(outputValues[i]);
    if(i<3) { Serial.print(","); }
  }
  Serial.println("");

  delay(50);
}
