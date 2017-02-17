/*
 * John Dix
 * Phys 222 Lab 5 - Winter 2017
 */

 // This is the debug flag that turns on debugging information if needed
 #define _DEBUG

 #define MYHIGH   3.3
 #define MYLOW    1.3

// LED Pinouts
const int pinDCMotor = 13;       // DC motor voltage


// Pre-execution initialisation occurs here
void setup() {
  Serial.begin(9600); 
}

// The main loop
void loop() {
  pulseDCMotor();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: pulseDCMotor
// Purpose: Pulses the DC motor
// Inputs: n/a
// Outputs: n/a
// Notes: n/a
//
void pulseDCMotor() {
  analogWrite(A0, 255);
}

