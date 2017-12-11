const int pinDigiLED = 11;
const int pinAnaLED = 9;

void setup() {
  pinMode(pinDigiLED, OUTPUT);
}

void loop() {
  // lab05_03();
  // lab05_05a();
  lab05_05b();
}

void lab05_03() {
  dblink(pinDigiLED, 1000);
}

void lab05_05a() {
  static int i = 0;
  dblink(pinDigiLED, 500);
  i++;
  if (i == 10) {
    delay(3000);
    i = 0;
  }
}

void lab05_05b() {
  static int i = 0;
  static int j = 255;
  ablink(pinAnaLED, 500, j);

  // Slowly drop the brightness then reset it
  j -= 50;
  if (j <= 0)
    j = 255;

  // After 10 blinks shutoff for 3s
  if (++i >= 10) {
    delay(3000);
    i = 0;
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
