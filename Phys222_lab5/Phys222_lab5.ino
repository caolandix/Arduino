/*
 * John Dix
 * Phys 222 Lab 5 - Winter 2017
 */

// LED Pinouts
const int pinDCMotor = 9;       // DC motor voltage


// Pre-execution initialisation occurs here
void setup() {
  Serial.begin(9600); 
  pinMode(pinDCMotor, OUTPUT);
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
  analogWrite(pinDCMotor, 255);
  delay(1000);
  analogWrite(pinDCMotor, 0);
  delay(1000);
}

