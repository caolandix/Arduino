
// define the pins to be used. 
const int pinDigiLED = 13;
const int pinAnaLED = 9;

void setup() {

  // Set up the Digital write on pin 
  pinMode(pinDigiLED, OUTPUT);

  // Setup the random seed for Assignment05_05b() otherwise you'll get the same values every execution
  randomSeed(analogRead(0));

  // Open the serial port for Assignment05_06() to output to Serial console.
  Serial.begin(9600);
}

// Main loop. COmment out functions as necessary to execute the code.
void loop() {
  // Assignment_03();
  Assignment_05a();       // <=== Currently executing this one.
  // Assignment05_05b();
  // Assignment05_06();
}

void Assignment_03() {
  dblink(pinDigiLED, 1000);
}

void Assignment_05a() {
  for (int i = 0; i < 10; i++) {
    dblink(pinDigiLED, 500);
  }
  delay(3000);
}

void Assignment05_05b() {
  for (int i = 0; i < 10; i++) {

    // Analog blink with random brightness values from 0..255
    ablink(pinAnaLED, 500, random() % 256);
  }
  delay(3000);
}

void Assignment05_06() {
  for (int i = 0; i < 256; i++) {
    Serial.println(i);    // Output to serial console to see when the LED first comes on. You may need adjust the "Port" settings in the Tools menu
    ablink(pinAnaLED, 30, i);
  }
}

// Blink using digital writes
void dblink(int pin, int milliseconds) {
  digitalWrite(pin, HIGH);
  delay(milliseconds);
  digitalWrite(pin, LOW);
  delay(milliseconds);
}

// blink using analog writes
void ablink(int pin, int milliseconds, int brightness) {
  analogWrite(pin, brightness);
  delay(milliseconds);
  analogWrite(pin, 0);
  delay(milliseconds);
}
