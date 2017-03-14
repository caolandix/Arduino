/*
Phys 222 - Winter 2017 - Rocket Arduino Project
Team Members:
  Adam Burtle
  Caolan Dix
  Riwen Mao
Purpose: Send up in a rocket a GPS unit that transmits Lat/Long and saves more extensive data to an SDMicro chip
*/

// Header files for preprocessing
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>

#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"

// turn on only the second sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"

// turn on GPRMC and GGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

// turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

// turn off output
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
#define PMTK_Q_RELEASE "$PMTK605*31"

const byte HC12RxdPin = 4;                      // "RXD" Pin on HC12
const byte HC12TxdPin = 5;                      // "TXD" Pin on HC12
const byte HC12SetPin = 6;                      // "SET" Pin on HC12
const byte GPSRxdPin = 7;                       // "RXD" on GPS (if available)
const byte GPSTxdPin = 8;                       // "TXD" on GPS

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

  Serial.begin(230400);
  setupGPS();
  // setupHC12();
}
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Purpose: 
// Inputs:
// Output: 
// Notes: 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void setupGPS() {
  // wait for leo to be ready
  while (!Serial)
    ;

  // Setup the SD for usage
  // Open ports and setup communication with SDChip
  //setupSD();
        
  GPS.begin(9600);
  delay(2000);
  GPS.println(PMTK_Q_RELEASE);
  
  // you can send various commands to get it started
  // GPS.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.println(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  GPS.println(PMTK_SET_NMEA_UPDATE_1HZ);
}
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Purpose: 
// Inputs:
// Output: 
// Notes: 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void setupHC12() {
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
  if (Serial.available()) {
   char c = Serial.read();
   Serial.write(c);
   GPS.write(c);
  }
  // handleHC12();
  //handleSerialBuffer();
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
    if (byteIn == '\n')                             // At the end of the line
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
    if (byteIn == '\n') {
      if (GPSLocal) {
        Serial.print(GPSReadBuffer);
        parseGPSSentence(GPSReadBuffer.c_str());
      }
      else {
        HC12.print("Remote GPS:");                  // Local Arduino responds to remote request
        HC12.print(GPSReadBuffer);                  // Sends local GPS to remote
      }
      // saveGPSDataSDCard();                          // Save GPS info to SDMicro
      // HC12.listen();                                // Found target GPS sentence, start listening to HC12 again
      GPSReadBuffer = "";                           // Delete unwanted strings
    }
  }  
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: parseHex
// Purpose: read a Hex value and return the decimal equivalent
// Inputs: N/A
// Output: N/A
// Notes:
//
uint8_t parseHex(const char c) {
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A') + 10;
    return 0;
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: checksumPassed
// Purpose: Looks for a checksum in the data stream to make sure it is a valid line in case there is a failure to receive a full sentence
// Inputs: N/A
// Output: N/A
// Notes:
//
bool checksumPassed(const char *GPSReadBuffer) {
  
  // first look if we even have one
  if (GPSReadBuffer[strlen(GPSReadBuffer) - 4] == '*') {

    // parse the data looking for a checksum
    uint16_t sum = parseHex(GPSReadBuffer[strlen(GPSReadBuffer) - 3]) * 16;
    sum += parseHex(GPSReadBuffer[strlen(GPSReadBuffer) - 2]);
    
    // check checksum 
    for (uint8_t i = 2; i < (strlen(GPSReadBuffer) - 4); i++)
      sum ^= GPSReadBuffer[i];
    if (sum != 0)
      return false;
  }
  return true;
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: parseGPSSentence
// Purpose: Parses out the GPS Sentences and extracts the meaningful data for display
// Inputs: 
//    - GPSReadBuffer: the buffer of raw data read in by the GPS module
// Output: N/A
// Notes:
//
bool parseGPSSentence(const char *GPSReadBuffer) {
  char *pBuffer = NULL;
  if (!GPSReadBuffer)
    return false;

  pBuffer = GPSReadBuffer;
  
  // Verify the data to see if we received a checksum
  if (checksumPassed(GPSReadBuffer)) {
        
    // Handle the $GPGGA string
    if (strstr(GPSReadBuffer, "$GPGGA"))
      parseGPGGA(pBuffer);
    else if (strstr(GPSReadBuffer, "$GPRMC"))
      parseGPRMC(pBuffer);
    return true;
  }
  return false;
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: parseGPRMC
// Purpose: Parses out the GPS GPRMC sentence and extracts the meaningful data for display
// Inputs: 
//    - pBuffer: the buffer of raw data read in by the GPS module
// Output: N/A
// Notes:
//
void parseGPRMC(const char *pBuffer) {
  uint8_t hour, minute, seconds;
  uint16_t milliseconds;
  bool bActive;
  char degreebuff[10];  
  char lat, lon, mag;
  float latitudeDegrees, longitudeDegrees, latitude, longitude;
  int32_t latitude_fixed, longitude_fixed, degree;
  long minutes;
  
  // get time
  pBuffer = strchr(pBuffer, ',') + 1;
  float timef = atof(pBuffer);
  uint32_t time = timef;
  hour = time / 10000;
  minute = (time % 10000) / 100;
  seconds = (time % 100);
  milliseconds = fmod(timef, 1.0) * 1000;

  // parse out Active (A) or void (V)
  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer)
    bActive = (pBuffer[0] == 'A') ? true : false;
  
  // parse out latitude
  pBuffer = strchr(pBuffer, ',')+1;
  if (',' != *pBuffer) {
    strncpy(degreebuff, pBuffer, 2);
    pBuffer += 2;
    degreebuff[2] = '\0';
    degree = atol(degreebuff) * 10000000;

    // minutes
    strncpy(degreebuff, pBuffer, 2);

    // skip decimal point
    pBuffer += 3;
    strncpy(degreebuff + 2, pBuffer, 4);
    degreebuff[6] = '\0';
    minutes = 50 * atol(degreebuff) / 3;
    latitude_fixed = degree + minutes;
    latitude = degree / 100000 + minutes * 0.000006F;
    latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
    latitudeDegrees += int(latitude/100);
  }
  
  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer) {
    if (pBuffer[0] == 'S')
      latitudeDegrees *= -1.0;
    if (pBuffer[0] == 'N')
      lat = 'N';
    else if (pBuffer[0] == 'S')
      lat = 'S';
    else if (pBuffer[0] == ',')
      lat = 0;
    else
      return false;
  }
  
  // parse out longitude
  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer) {
    strncpy(degreebuff, pBuffer, 3);
    pBuffer += 3;
    degreebuff[3] = '\0';
    degree = atol(degreebuff) * 10000000;

    // minutes
    strncpy(degreebuff, pBuffer, 2);

    // skip decimal point
    pBuffer += 3;
    strncpy(degreebuff + 2, pBuffer, 4);
    degreebuff[6] = '\0';
    minutes = 50 * atol(degreebuff) / 3;
    longitude_fixed = degree + minutes;
    longitude = degree / 100000 + minutes * 0.000006F;
    longitudeDegrees = (longitude - 100 * int(longitude / 100)) / 60.0;
    longitudeDegrees += int(longitude / 100);
  }
  
  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer) {
    if (pBuffer[0] == 'W')
      longitudeDegrees *= -1.0;
    if (pBuffer[0] == 'W')
      lon = 'W';
    else if (pBuffer[0] == 'E')
      lon = 'E';
    else if (pBuffer[0] == ',')
      lon = 0;
    else
      return false;
  }    
}
/*
$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A

Where:
     RMC          Recommended Minimum sentence C
     123519       Fix taken at 12:35:19 UTC
     A            Status A=active or V=Void.
     4807.038,N   Latitude 48 deg 07.038' N
     01131.000,E  Longitude 11 deg 31.000' E
     022.4        Speed over the ground in knots
     084.4        Track angle in degrees True
     230394       Date - 23rd of March 1994
     003.1,W      Magnetic Variation
     *6A          The checksum data, always begins with *
*/
void parseGPGGA(const char *pBuffer) {
  float geoIDHeight, altitude, HDOP, latitudeDegrees, longitudeDegrees, latitude, longitude;
  char degreebuff[10];  
  char lat, lon, mag;
  uint8_t hour, minute, seconds, year, month, day, fixquality, satellites;
  uint16_t milliseconds;
  int32_t latitude_fixed, longitude_fixed, degree;
  long minutes;

  // get time
  pBuffer = strchr(pBuffer, ',') + 1;
  float timef = atof(pBuffer);
  uint32_t time = timef;
  hour = time / 10000;
  minute = (time % 10000) / 100;
  seconds = (time % 100);

  milliseconds = fmod(timef, 1.0) * 1000;

  // parse out latitude
  pBuffer = strchr(pBuffer, ',')+1;
  if (',' != *pBuffer) {
    strncpy(degreebuff, pBuffer, 2);
    pBuffer += 2;
    degreebuff[2] = '\0';
    degree = atol(degreebuff) * 10000000;

    // minutes
    strncpy(degreebuff, pBuffer, 2);

    // skip decimal point
    pBuffer += 3;
    strncpy(degreebuff + 2, pBuffer, 4);
    degreebuff[6] = '\0';
    minutes = 50 * atol(degreebuff) / 3;
    latitude_fixed = degree + minutes;
    latitude = degree / 100000 + minutes * 0.000006F;
    latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
    latitudeDegrees += int(latitude/100);
  }
  
  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer) {
    if (pBuffer[0] == 'S')
      latitudeDegrees *= -1.0;
    if (pBuffer[0] == 'N')
      lat = 'N';
    else if (pBuffer[0] == 'S')
      lat = 'S';
    else if (pBuffer[0] == ',')
      lat = 0;
    else
      return false;
  }
  
  // parse out longitude
  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer) {
    strncpy(degreebuff, pBuffer, 3);
    pBuffer += 3;
    degreebuff[3] = '\0';
    degree = atol(degreebuff) * 10000000;

    // minutes
    strncpy(degreebuff, pBuffer, 2);

    // skip decimal point
    pBuffer += 3;
    strncpy(degreebuff + 2, pBuffer, 4);
    degreebuff[6] = '\0';
    minutes = 50 * atol(degreebuff) / 3;
    longitude_fixed = degree + minutes;
    longitude = degree / 100000 + minutes * 0.000006F;
    longitudeDegrees = (longitude - 100 * int(longitude / 100)) / 60.0;
    longitudeDegrees += int(longitude / 100);
  }
  
  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer) {
    if (pBuffer[0] == 'W')
      longitudeDegrees *= -1.0;
    if (pBuffer[0] == 'W')
      lon = 'W';
    else if (pBuffer[0] == 'E')
      lon = 'E';
    else if (pBuffer[0] == ',')
      lon = 0;
    else
      return false;
  }

  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer)
    fixquality = atoi(pBuffer);

  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer)
    satellites = atoi(pBuffer);

  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer)
    HDOP = atof(pBuffer);
  
  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer)
    altitude = atof(pBuffer);
  
  pBuffer = strchr(pBuffer, ',') + 1;
  pBuffer = strchr(pBuffer, ',') + 1;
  if (',' != *pBuffer)
    geoIDHeight = atof(pBuffer);

  Serial.print("Latitude (deg): ");
  Serial.print(latitudeDegrees);
  Serial.println(lat);
  Serial.print("Longitude (deg): ");
  Serial.print(longitudeDegrees);
  Serial.println(lon);  
  Serial.print("geoIDHeight: ");
  Serial.println(geoIDHeight);
  Serial.print("Altitude (m): ");
  Serial.println(altitude);
  Serial.print("fixquality: ");
  Serial.println(fixquality);
  Serial.print("satellites: ");
  Serial.println(satellites);
  Serial.print("HDOP: ");
  Serial.println(HDOP);
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
