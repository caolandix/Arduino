/*
Phys 222 - Winter 2017 - Rocket Arduino Project
Team Members:
  Adam Burtle
  Caolan Dix
  Fangping Zhou
  Gianna Cox
  Riwen Mao
Purpose: Send up in a rocket a GPS unit that transmits Lat/Long and saves more extensive data to an SDMicro chip
*/

// Header files for preprocessing
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>

// SD Logging basics -- Chip Select
// Stolen from the net so pins are as follows - this will have to be changed
// SD card attached to SPI bus as follows:
// MOSI - pin 11
// MISO - pin 12
// CLK - pin 13
// CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN) (chipSelect = CS)
const byte chipSelect = 9;

// Begin Pin Declarations
const byte HC12RxdPin = 4;                          // "RXD" Pin on HC12
const byte HC12TxdPin = 5;                          // "TXD" Pin on HC12
const byte HC12SetPin = 6;                          // "SET" Pin on HC12
const byte GPSRxdPin = 7;                          // "RXD" on GPS (if available)
const byte GPSTxdPin = 8;                          // "TXD" on GPS

// Begin variable declarations
char byteIn;                                        // Temporary variable
String HC12ReadBuffer = "";                         // Read/Write Buffer 1 -- Serial
String SerialReadBuffer = "";                       // Read/Write Buffer 2 -- HC12
String GPSReadBuffer = "";                          // Read/Write Buffer 3 -- GPS
bool serialEnd = false;                          // Flag for End of Serial String
bool HC12End = false;                            // Flag for End of HC12 String
bool GPSEnd = false;                             // Flag for End of GPS String
bool commandMode = false;                        // Send AT commands to remote receivers
bool GPSLocal = true;                            // send GPS local or remote flag

// Create Software Serial Ports for HC12 & GPS - Software Serial ports Rx and Tx are opposite the HC12 Rxd and Txd
SoftwareSerial HC12(HC12TxdPin, HC12RxdPin);
SoftwareSerial GPS(GPSTxdPin, GPSRxdPin);

// Function prototypes -- forward declarations to tell the compiler that they're here... Not really needed in Arduino but compiler settings may change in the future. 
void handleHC12();
void handleSerialBuffer();
void handleGPS();
void setupSD();
void saveGPSDataSDCard();

//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: setup
// Purpose: Setsup all preliminary initialization needed before entering the primary loop()
// Inputs: N/A
// Output: N/A
// Notes:
//
void setup() {
  HC12ReadBuffer.reserve(82);                       // Reserve 82 bytes for message
  SerialReadBuffer.reserve(82);                     // Reserve 82 bytes for message
  GPSReadBuffer.reserve(82);                        // Reserve 82 bytes for longest NMEA sentence

  Serial.begin(9600);                               // Open serial port to computer at 9600 Baud

  // Setup the SD for usage
  setupSD();                                        // Open ports and setup communication with SDChip

  // Setup the GPS unit for usage
  GPS.begin(9600);                                  // Open software serial port to GPS at 9600 Baud

  // Setup the HC12 unit for usage
  pinMode(HC12SetPin, OUTPUT);                      // Output High for Transparent / Low for Command
  digitalWrite(HC12SetPin, HIGH);                   // Enter Transparent mode
  delay(80);                                        // 80 ms delay before operation per datasheet
  HC12.begin(9600);                                 // Open software serial port to HC12 at 9600 Baud
  HC12.listen();                                    // Listen to HC12
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: loop
// Purpose: The main loop of execution
// Inputs: N/A
// Output: N/A
// Notes: 
//
void loop() {
  handleHC12();
  handleSerialBuffer();
  handleGPS();
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: handleHC12
// Purpose: Reads and manages the data coming from the readHC12
// Inputs: N/A
// Output: N/A
// Notes:
//
void handleHC12() {
  while (HC12.available()) {                        // If Arduino's HC12 rx buffer has data
    byteIn = HC12.read();                           // Store each character in byteIn
    HC12ReadBuffer += char(byteIn);                 // Write each character of byteIn to HC12ReadBuffer
    if (byteIn == '\n')                             // At the end of the line
      HC12End = true;                               // Set HC12End flag to true.
  }
  if (HC12End) {                                    // If HC12End flag is true
    if (HC12ReadBuffer.startsWith("AT")) {          // Check to see if a command was received
      digitalWrite(HC12SetPin, LOW);                // If true, enter command mode
      delay(40);                                    // Delay before writing command
      HC12.print(HC12ReadBuffer);                   // Send incoming command back to HC12
      Serial.println(HC12ReadBuffer);               // Send command to serial
      delay(1000);                                  // Wait 0.5s for reply
      digitalWrite(HC12SetPin, HIGH);               // Exit command / enter transparent mode
      delay(80);                                    // Delay before proceeding
      HC12.println("Remote Command Executed");
    }
    if (HC12ReadBuffer.startsWith("GPS")) {
      GPS.listen();
      HC12.print("Remote GPS Command Received");
      GPSLocal = false;
    }
    Serial.print(HC12ReadBuffer);                   // Send message to screen
    HC12ReadBuffer = "";                            // Empty Buffer
    HC12End = false;                                // Reset Flag
  }  
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: handleSerialBuffer
// Purpose: Reads and manages the data coming from the serial buffer used for communication between the the GPS and the radio HC12
// Inputs:
// Output: N/A
// Notes:
//
void handleSerialBuffer() {
  while (Serial.available()) {                      // If Arduino's computer rx buffer has data
    byteIn = Serial.read();                         // Store each character in byteIn
    SerialReadBuffer += char(byteIn);               // Write each character of byteIn to SerialReadBuffer
    if (byteIn == '\n')                          // At the end of the line
      serialEnd = true;                             // Set serialEnd flag to true.
  }
  if (serialEnd) {                                  // Check to see if serialEnd flag is true
    if (SerialReadBuffer.startsWith("AT")) {        // Check to see if a command has been sent
      if (SerialReadBuffer.startsWith("AT+B")) {    // If it is a baud change command, delete it immediately
        SerialReadBuffer = "";
        Serial.print("Denied: Changing HC12 Baud does not change Arduino Baudrate");
      }
      HC12.print(SerialReadBuffer);                 // Send local command to remote HC12 before changing settings
      delay(100);                                   //
      digitalWrite(HC12SetPin, LOW);                // If true, enter command mode
      delay(100);                                   // Delay before writing command
      HC12.print(SerialReadBuffer);                 // Send command to HC12
      Serial.print(SerialReadBuffer);               // Send command to serial
      delay(500);                                   // Wait 0.5s for reply
      digitalWrite(HC12SetPin, HIGH);               // Exit command / enter transparent mode
      delay(100);                                   // Delay before proceeding
    }
    if (SerialReadBuffer.startsWith("GPS")) {
      GPS.listen();
      GPSLocal = true;
    }
    HC12.print(SerialReadBuffer);                   // Send text to HC12 to be broadcast
    SerialReadBuffer = "";                          // Clear buffer 2
    serialEnd = false;                              // Reset serialEnd flag
  }  
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: handleGPS
// Purpose: Reads and manages the data coming from the GPS unit
// Inputs: N/A
// Output: N/A
// Notes:
//
void handleGPS() {
  while (GPS.available()) {
    byteIn = GPS.read();
    GPSReadBuffer += char(byteIn);
    if (byteIn == '\n')
      GPSEnd = true;
  }
  if (GPSEnd) {
    
    // Options include GPRMC, GPGGA, GPGLL, etc...
    if (!GPSReadBuffer.startsWith("$GPGGA")) {       // Look for target GPS sentence
      if (GPSLocal) {
        Serial.print("Local  GPS:");                // Send to local serial port
        Serial.print(GPSReadBuffer);                // Send local GPS
      }
      else {
        HC12.print("Remote GPS:");                  // Local Arduino responds to remote request
        HC12.print(GPSReadBuffer);                  // Sends local GPS to remote
      }
      saveGPSDataSDCard();                          // Save GPS info to SDMicro
      HC12.listen();                                // Found target GPS sentence, start listening to HC12 again
    }
    GPSReadBuffer = "";                           // Delete unwanted strings
    GPSEnd = false;                                 // Reset GPS
  }  
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: setupSD
// Purpose: Initialises the communication channel to the SD Card. 
// Inputs: N/A
// Output: N/A
// Notes: This is stub code TBH. Because the SDCard is actually a part of the GPS unit, more than likely it will require different
//        processing however for now it is here to allow development to proceed.
//
void setupSD() {

  // see if the card is present and can be initialized:
  Serial.print("Initializing SD card...");
  if (SD.begin(chipSelect))
    Serial.println("card initialized.");
  else
    Serial.println("Card failed, or not present");
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: saveGPSDataSDCard
// Purpose: Writes the GPS information to the SDCard
// Inputs: N/A
// Output: N/A
// Notes:This is stub code TBH. Because the SDCard is actually a part of the GPS unit, more than likely it will require different
//        processing however for now it is here to allow development to proceed.
//
void saveGPSDataSDCard() {
  // make a string for assembling the data to log:
  String dataString = "";
  String strFilename = "gps_datalog.txt";

  // read three sensors and append to the string:
  for (int analogPin = 0; analogPin < 3; analogPin++) {
    int sensor = analogRead(analogPin);
    dataString += String(sensor);
    if (analogPin < 2)
      dataString += ",";
  }

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(strFilename, FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else
    Serial.print("error opening ");
    Serial.println(strFilename);
}
