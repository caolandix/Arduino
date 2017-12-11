#include <util/delay_basic.h>

#ifdef ARDUINO_AVR_MEGA2560
  #define LAMP 0x20
  #define SH 0x40
  #define ICG 0x80
  #define MCLK 0x10
#else
  #define LAMP 0x01
  #define SH 0x02
  #define ICG 0x04
  #define MCLK 0x08
#endif

#define CLOCK PORTB

uint8_t buffer[800];
uint8_t avg = 0;
char cmdBuffer[16];
int cmdIndex;
int exposureTime = 20;

void setup() {
  uint8_t val;

  // Initialize the clocks.
  DDRB |= (LAMP | SH | ICG | MCLK);   // Set the clock lines to outputs
  CLOCK |= ICG;                       // Set the integration clear gate high.

  // Enable the serial port.
  Serial.begin(115200);

  // Setup timer2 to generate a 470kHz frequency on D11
  TCCR2A =  + (0 << COM2A1) | (1 << COM2A0) | (1 << WGM21) | (0 << WGM20);
  TCCR2B = (0 << WGM22) | (1 << CS20);
  OCR2A = 20;
  TCNT2 = 1;

  // Set the ADC clock to sysclk/32
  ADCSRA &= ~((1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0));
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0);
}

void readCCD(void) {
  uint8_t result;

  CLOCK &= ~ICG;
  _delay_loop_1(12);
  CLOCK |= SH;
  delayMicroseconds(5);
  CLOCK &= ~SH;
  delayMicroseconds(15);
  CLOCK |= ICG;
  delayMicroseconds(1);

  for (int x = 0; x < 800; x++) {
    CLOCK |= SH;
    if (x == 0) {
      avg = (uint8_t)(analogRead(A0) >> 2);
      result = (uint8_t)(analogRead(A0) >> 2);
    }
    else {
      result = (uint8_t)(analogRead(A0) >> 2);
      result = (result < avg) ? 0 : result - avg;
      buffer[x] = result;
      delayMicroseconds(20);
    }
    CLOCK &= ~SH;
  }
}

uint16_t centroid() {
  uint32_t sum = 0;
  uint32_t so_far = 0;
  uint32_t half_max;

  for (uint16_t x = 0; x < sizeof(buffer); ++x)
    sum += buffer[x];
  half_max = sum / 2;
  for (uint16_t x = 0; x < sizeof(buffer); ++x) {
    so_far += buffer[x];
    if (so_far >= half_max)
      return x;
  }
}

void sendData(void) {
  for (int x = 0; x < 800; ++x) {
    if (x % 20 == 0)
      Serial.println("\n");
    Serial.print(buffer[x]);
  }
}

void loop() {
  if (Serial.available())
    cmdBuffer[cmdIndex++] = Serial.read();
  switch (cmdBuffer[0]) {
    case 'r':
      sendData();
      break;
    case 'l':
      CLOCK &= ~LAMP;
      break;
    case 'L':
      CLOCK |= LAMP;
      break;
    case 'e':
      if (--exposureTime < 0)
        exposureTime = 0;
      Serial.print("Exposure time ");
      Serial.println(exposureTime);
      break;
    case 'E':
      if (++exposureTime > 200)
        exposureTime = 200;
      Serial.print("Exposure time ");
      Serial.println(exposureTime);      
      break;
    case 'c':
      Serial.print("Centroid position: ");
      Serial.println(centroid());    
      break;
  }
  cmdBuffer[0] = '\0';
  cmdIndex = 0;

  readCCD();
  delay(exposureTime);
}
