/*
 * John Dix
 * Phys 222 Lab 5 - Winter 2017
 */

 // This is the debug flag that turns on debugging information if needed
 #define _DEBUG

// TMP36 Pin Variables
const int sensorPin = A0;       // the analog pin the TMP36's Vout (sense) pin is connected to the resolution is 10 mV / degree centigrade with a 500 mV offset to allow for negative temperatures

// LED Pinouts
const int ledHotPin = 13;       // Red LED pin for temps greater than 25C
const int ledWarmPin = 9;       // Yellow LED pin for temps in the comfortable range 
const int ledColdPin = 7;       // Blue LED pin for temps less than 10C

// Temperature ranges for red and blue LED's
const float tempHot = 25.0;
const float tempCold = 10.0;

// make it easier to type in unsigned long in the program
typedef unsigned long ulong;

// Pre-execution initialisation occurs here
void setup() {
  Serial.begin(9600);
  pinMode(ledHotPin, OUTPUT);
  pinMode(ledWarmPin, OUTPUT);
  pinMode(ledColdPin, OUTPUT);
  digitalWrite(ledHotPin, LOW);
  digitalWrite(ledWarmPin, LOW);
  digitalWrite(ledColdPin, LOW);  
}

// The main loop
void loop() {
  logTemps();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: logTemps
// Purpose: Reads temperature data from temp sensor on arduino, does calculations, and activates LED's as necessary.
// Inputs: n/a
// Outputs: n/a
// Notes: n/a
//
void logTemps() {
  ulong currTime = millis();
  
  // getting the voltage reading from the temperature sensor.
  float rawData = analogRead(sensorPin);

  // Adjust according to lab info
  float adjData = (rawData / 1023.0) * 5.0;

  // Convert to Celcius
  float currCelcius = (adjData / 0.01) - 50.0;
  static float celcius10SecAgo = currCelcius;

  // Convert to Fahrenheit
  float fahrenheit = (currCelcius * 9.0 / 5.0) + 32.0;

  // Conditions to turn on the LED's...
  // If warmer than 25.0 Celcius, turn the red LED on.
  if (currCelcius > tempHot) {
    digitalWrite(ledHotPin, HIGH);
    digitalWrite(ledWarmPin, LOW); 
  }

  // if colder than 10.0 Celcius, turn on the blue LED
  else if (currCelcius < tempCold) {
    digitalWrite(ledColdPin, HIGH);
    digitalWrite(ledWarmPin, LOW); 
  }

  // Not warm enough for red, not cold enough for blue
  else {
      digitalWrite(ledHotPin, LOW);
      digitalWrite(ledColdPin, LOW); 
      digitalWrite(ledWarmPin, HIGH); 
  }

  // Output our readings -- If debugging the application, output to the serial monitor a more verbose set of data
  #ifdef _DEBUG
  Serial.println("\t***********************************************");
  Serial.print("\tCurrent Time (s): \t\t");
  Serial.println(currTime / 1000.0);
  Serial.print("\tRaw Reading: \t\t\t");
  Serial.println(rawData);
  Serial.print("\tAdjusted Reading: \t\t");
  Serial.println(adjData);
  Serial.print("\tTemperature (C): \t\t");
  Serial.println(currCelcius);
  Serial.print("\tTemperature (F): \t\t");
  Serial.println(fahrenheit);
  
  // If not debugging and using actual release code (smaller footprint) then just print the data we need
  #else
  Serial.print(currTime);
  Serial.print(",");
  Serial.println(currCelcius); 
  #endif

  // 1 second delay
  delay(1000);
}
