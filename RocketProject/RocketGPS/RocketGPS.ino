/*
Phys 222 - Winter 2017 - Rocket Arduino Project
Team Members:
  Adam Burtle
  Caolan Dix
  Riwen Mao
Purpose: Send up in a rocket a GPS unit that transmits Lat/Long and saves more extensive data to an SDMicro chip

NOTES: Although I based this code on samples for the adafruit GPS unit, it is mostly all original code.

NMEA Sentences are documented here: http://www.gpsinformation.org/dale/nmea.htm
*/

// Header files for preprocessing
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>

// 
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

// The field delimiter in NMEA sentences
#define NMEA_DELIMITER  ','

// SD related settings
#define LOG_FIXONLY false  

//
// Structure used to hold information about the position
//
struct GPSInfo {
  String strLatitude;     // Latitude... hurrrdurrr
  String strLongitude;    // Uhm, longitude 
  String strTimestamp;    // Timestamp
  String strDate;         // The Current Date
  String strSpeed;        // Speed in knots
  String strStatus;       // Active or Void -- Not sure what this means but it's a part of the NMEA definition
  String strTrackingAngle;
};

// Error types we report on
enum { ERR_UNKNOWN = 1, ERR_SD_INIT, ERR_SD_CREATEFILE, ERR_SD_WRITEFILE};

// HW pins used int he Arduino
const byte HC12RxdPin = 4;                      // "RXD" Pin on HC12 -- Unused
const byte HC12TxdPin = 5;                      // "TXD" Pin on HC12 -- Unused
const byte HC12SetPin = 6;                      // "SET" Pin on HC12 -- Unused
const byte GPSRxdPin = 7;                       // "RXD" on GPS (if available)
const byte GPSTxdPin = 8;                       // "TXD" on GPS
const byte GPSchipSelect = 10;                  // ChipSelect for the SD
const byte GPSledPin = 13;                      // LED Pin for the GPS shield

// Begin variable declarations
char byteIn;                                        // Temporary variable
String HC12ReadBuffer = "";                         // Read/Write Buffer 1 -- Serial -- Unused
String SerialReadBuffer = "";                       // Read/Write Buffer 2 -- HC12 -- Unused
String GPSReadBuffer = "";                          // Read/Write Buffer 3 -- GPS
bool serialEnd = false;                          // Flag for End of Serial String -- Unused
bool HC12End = false;                            // Flag for End of HC12 String -- Unused
bool GPSEnd = false;                             // Flag for End of GPS String
bool commandMode = false;                        // Send AT commands to remote receivers -- Unused
bool GPSLocal = true;                            // send GPS local or remote flag
GPSInfo g_gpsInfo;                              // GLobal structure used to hold GPS information
File logfile;                                   // The global file descriptor for the logfile
uint8_t timeZone = -8;                          // PST -- Unused


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
  //HC12ReadBuffer.reserve(82);                       // Reserve 82 bytes for message
  //SerialReadBuffer.reserve(82);                     // Reserve 82 bytes for message
  GPSReadBuffer.reserve(82);                        // Reserve 82 bytes for longest NMEA sentence

  // Setup the serial port to be fast enough to handle the incoming data from the GPS and give it time to process it
  Serial.begin(115200);
  setupSD();
  setupGPS();
  // setupHC12();
}
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: blinkError()
// Purpose: Sends out an error code to the error LED on the GPS board
// Inputs: 
//    - errno: the error code being registered
// Output: N/A
// Notes: 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void handleBlinkError(uint8_t errCode, const char *pFilename = NULL) {  
  switch (errCode) {
    case ERR_SD_INIT:
      Serial.println("ERROR: SD Card: initialisation failed!");
      break;
    case ERR_SD_CREATEFILE:
      if (pFilename) {
        Serial.print("ERROR: SD Card: Failed to create a file: ");
        Serial.println(pFilename);
      }
      break;
    case ERR_SD_WRITEFILE:
      if (pFilename) {
        Serial.print("ERROR: SD Card: Failed to write to a file: ");
        Serial.println(pFilename);
      }
      break;
    default:
      Serial.println("ERROR: Unknown error occurred");
  }

  // set the blinking code
  while (true) {
    for (uint8_t i = 0; i < errCode; i++) {
      digitalWrite(GPSledPin, HIGH);
      delay(100);
      digitalWrite(GPSledPin, LOW);
      delay(100);
    }
    for (uint8_t i = errCode; i < 10; i++) {
      delay(200);
    }
  }
}
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: setupSD()
// Purpose: initialises the SD
// Inputs: N/A
// Output: N/A
// Notes: 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void setupSD() {
  pinMode(GPSledPin, OUTPUT);

  // make sure that the default chip select pin is set to output, even if you don't use it:
  pinMode(GPSchipSelect, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(GPSchipSelect, 11, 12, 13)) {
    handleBlinkError(ERR_SD_INIT);
  }
  char filename[15];
  strcpy(filename, "GPSLOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i / 10;
    filename[7] = '0' + i % 10;
    
    // create if does not exist, do not open existing, write, sync after write
    if (!SD.exists(filename))
      break;
  }

  logfile = SD.open(filename, FILE_WRITE);
  if (!logfile)
    handleBlinkError(ERR_SD_CREATEFILE, filename);
  Serial.print("Writing to ");
  Serial.println(filename);
}
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: setupGPS()
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
  GPS.begin(9600);
  delay(2000);
  GPS.println(PMTK_Q_RELEASE);
  
  // you can send various commands to get it started
  // GPS.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.println(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  GPS.println(PMTK_SET_NMEA_UPDATE_1HZ);
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
        parseGPSSentence(GPSReadBuffer.c_str());
        flushSDBuffer(GPSReadBuffer.c_str());
      }
      else {
        //HC12.print("Remote GPS:");                  // Local Arduino responds to remote request
        // HC12.print(GPSReadBuffer);                  // Sends local GPS to remote
      }
      // HC12.listen();                                // Found target GPS sentence, start listening to HC12 again
      GPSReadBuffer = "";                           // Delete unwanted strings
    }
  }  
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: saveGPSDataSDCard
// Purpose: Writes the GPS information to the SDCard
// Inputs: N/A
// Output: N/A
// Notes:
//
void flushSDBuffer(const char *pBuffer) {
  if (strstr(pBuffer, "RMC"))
    logfile.flush();
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: saveGPSDataSDCard
// Purpose: Writes the GPS information to the SDCard
// Inputs: N/A
// Output: N/A
// Notes:
//
void saveGPSDataSDCard() {
  writeGPSInfoToSD();
  writeCRLFToSD();
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: writeGPSInfoToSD
// Purpose: Writes the GPS Info out to the SD
// Inputs: N/A
// Output: N/A
// Notes:
//
void writeGPSInfoToSD() {
  char *pBuffer = (String("Date: ") + g_gpsInfo.strDate).c_str();
  int strlength = strlen(pBuffer);    
  if (strlength != logfile.write((uint8_t *)pBuffer, strlength))    //write the string to the SD file
    handleBlinkError(ERR_SD_WRITEFILE);
  Serial.println(pBuffer);

  sprintf(pBuffer, "Date: %s", g_gpsInfo.strDate.c_str());
  
  char *pBuffer = new char[strlen(_gpsInfo.strDate.c_str()) + strlen("Date: ")];
  strncpy(pBuffer, gpsInfo.strDate.c_str(), strlen(gpsInfo.strDate.c_str()));
  strncpy(pBuffer + strlen(gpsInfo.strDate.c_str(), "Date: ", strlen("Date: "));
  delete [] pBuffer;
  
      
  //writeStringToSD((String("Date: ") + g_gpsInfo.strDate).c_str());
  writeStringToSD((String("Timestamp: ") + g_gpsInfo.strTimestamp).c_str());
  writeStringToSD(" ");
  writeStringToSD((String("Position: ") + String(g_gpsInfo.strLatitude) + String(", ") + String(g_gpsInfo.strLongitude)).c_str());
  writeStringToSD(" ");
  writeStringToSD((String("Speed: ") + g_gpsInfo.strSpeed + String("knts")).c_str());
  writeStringToSD(" ");
  //writeStringToSD(g_gpsInfo.strTrackingAngle.c_str());
  
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: writeStringToSD
// Purpose: Writes a string to the SD
// Inputs: 
//    - pBuffer: a pointer to the string to write
// Output: N/A
// Notes:
//
void writeStringToSD(char *pBuffer) {
  int strlength = strlen(pBuffer);    
  if (strlength != logfile.write((uint8_t *)pBuffer, strlength))    //write the string to the SD file
    handleBlinkError(ERR_SD_WRITEFILE);
}
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: writeCRLFToSD
// Purpose: Writes a CR/LF to the SD
// Inputs: N/A
// Output: N/A
// Notes:
//
void writeCRLFToSD() {
  char *pBuffer = "\n";
  uint8_t strlength = 0;
  
  strlength = strlen(pBuffer);    
  if (strlength != logfile.write((uint8_t *)pBuffer, strlength))
    handleBlinkError(ERR_SD_WRITEFILE);
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

  // if we have an invalid pointer then exit because it's an error
  if (!GPSReadBuffer)
    return false;

  pBuffer = GPSReadBuffer;
  
  // Verify the data to see if we received a checksum
  if (checksumPassed(GPSReadBuffer)) {
        
    // Handle the $GPRMC string
    if (strstr(GPSReadBuffer, "$GPRMC"))
      parseGPRMC(pBuffer);
  /*
    // Handle the $GPGGA string
    else if (strstr(GPSReadBuffer, "$GPGGA"))
      parseGPGGA(pBuffer);
      */
    return true;
  }
  return false;
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: parseTime
// Purpose: Parses out the timestamp of the GPS sentence
// Inputs: 
//    - ppBuffer: a pointer to the buffer of raw data read in by the GPS module
// Output: String of the timestamp
// Notes: 
//
String parseTime(char **ppBuffer) {
  uint8_t hour, minute, seconds;
  uint16_t milliseconds;
  float timef;
  uint32_t time;

  *ppBuffer = strchr(*ppBuffer, NMEA_DELIMITER) + 1;
  timef = atof(*ppBuffer);
  time = timef;
  hour = time / 10000;
  minute = (time % 10000) / 100;
  seconds = (time % 100);
  milliseconds = fmod(timef, 1.0) * 1000;       // Ignored for now
  return String(hour) + String(":") + String(minute) + String(":") + String(seconds);
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: parseLatLong
// Purpose: Parses out the latitude or longitude of the the buffer
// Inputs: 
//    - ppBuffer: a pointer to the buffer of raw data read in by the GPS module
//    - bIsLatitude: true we're doing lat, false is longitude
// Output: String of the Lat or Long
// Notes: 
//
String parseLatLong(char **ppBuffer, bool bIsLatitude) {
  char szBuffer[10];  
  char compassDir;
  float position;
  float latlongDegrees;
  int32_t position_fixed, degree;
  long minutes;
  uint8_t idxAdv;       // Index advancer

  // Go past the NMEA_DELIMITER
  *ppBuffer = strchr(*ppBuffer, NMEA_DELIMITER) + 1;
  
  if (NMEA_DELIMITER != **ppBuffer) {
    idxAdv = (bIsLatitude) ? 2 : 3;
    strncpy(szBuffer, *ppBuffer, idxAdv);
    *ppBuffer += idxAdv;
    szBuffer[idxAdv] = '\0';
    
    degree = atol(szBuffer) * 10000000;
    strncpy(szBuffer, *ppBuffer, 2);

    // skip decimal point
    *ppBuffer += 3;

    // Build up the degrees.minutes
    strncpy(szBuffer + 2, *ppBuffer, 4);
    szBuffer[6] = '\0';
    minutes = 50 * atol(szBuffer) / 3;
    position_fixed = degree + minutes;
    position = degree / 100000 + minutes * 0.000006F;
    latlongDegrees = (position - 100 * int(position / 100)) / 60.0;
    latlongDegrees += int(position / 100);
  }

  // Go to the compassDir field...
  *ppBuffer = strchr(*ppBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != **ppBuffer) {
    if (bIsLatitude) {
      if (**ppBuffer == 'S')
        latlongDegrees *= -1.0;
      compassDir = (**ppBuffer == 'N') ? 'N' : 'S';
    }
    else {
      if (**ppBuffer == 'W')
        latlongDegrees *= -1.0;
      compassDir = (**ppBuffer == 'W') ? 'W' : 'E'; 
    }
  }
  return String(latlongDegrees) + String(compassDir);
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: parseSpeed
// Purpose: Parses out the speed from a NMEA GPS Sentence
// Inputs: 
//    - pBuffer: the buffer of raw data read in by the GPS module
// Output: N/A
// Notes: 
//
String parseSpeed(char **ppBuffer) {
  float speed;

  *ppBuffer = strchr(*ppBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != **ppBuffer)
    speed = atof(*ppBuffer);
  return String(speed);
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: parseTrackingAngle
// Purpose: Parses out the tracking angle from a NMEA GPS Sentence
// Inputs: 
//    - ppBuffer: the buffer of raw data read in by the GPS module
// Output: String of converted value
// Notes: 
//
String parseTrackingAngle(char **ppBuffer) {
  float trackingAngle;

  *ppBuffer = strchr(*ppBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != **ppBuffer)
    trackingAngle = atof(*ppBuffer);
  return String(trackingAngle);
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: parseDate
// Purpose: Parses out the Date from a NMEA GPS Sentence
// Inputs: 
//    - ppBuffer: the buffer of raw data read in by the GPS module
// Output: String of converted value
// Notes: 
//
String parseDate(char **ppBuffer) {
  char szBuffer[6];
  
  *ppBuffer = strchr(*ppBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != **ppBuffer) {
    strncpy(szBuffer, *ppBuffer, 6);
    *ppBuffer += 6;
    szBuffer[6] = '\0';
  }

  // Got the date values and now need to separate into a date format of YY/MM/DD
  char year[2], month[2], day[2];

  Serial.println(szBuffer);
  strncpy(month, szBuffer, 2);
  month[2] = '\0';
  strncpy(day, szBuffer + 2, 2);
  day[2] = '\0';
  strncpy(year, szBuffer + 4, 2);
  year[2] = '\0';
  Serial.println(month);
  Serial.println(day);
  Serial.println(year);  

  return String(year) + String("/") + String(month) + String("/") + String(day);
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
void parseGPRMC(const char *pBuffer) {
  bool bActive;
  
  // get time
  g_gpsInfo.strTimestamp = parseTime(&pBuffer);

  // parse out Active (A) or void (V)
  pBuffer = strchr(pBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != *pBuffer)
    g_gpsInfo.strStatus = (*pBuffer == 'A') ? String("Active") : String("Void");
  
  // parse out latitude
  g_gpsInfo.strLatitude = parseLatLong(&pBuffer, true);
  
  // parse out longitude
  g_gpsInfo.strLongitude = parseLatLong(&pBuffer, false);

  // parse out the speed
  g_gpsInfo.strSpeed = parseSpeed(&pBuffer);

  // parse out the trackingAngle
  //g_gpsInfo.strTrackingAngle = parseTrackingAngle(&pBuffer);

  // parse out the Date
  // g_gpsInfo.strDate = parseDate(&pBuffer);
  saveGPSDataSDCard();  
}
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: parseGPGGA
// Purpose: Parses out the GPS Sentence GPGGA and extracts the meaningful data for display
// Inputs: 
//    - GPSReadBuffer: the buffer of raw data read in by the GPS module
// Output: N/A
// Notes:
//
/*
     GGA          Global Positioning System Fix Data
     123519       Fix taken at 12:35:19 UTC
     4807.038,N   Latitude 48 deg 07.038' N
     01131.000,E  Longitude 11 deg 31.000' E
     1            Fix quality: 0 = invalid
                               1 = GPS fix (SPS)
                               2 = DGPS fix
                               3 = PPS fix
             4 = Real Time Kinematic
             5 = Float RTK
                               6 = estimated (dead reckoning) (2.3 feature)
             7 = Manual input mode
             8 = Simulation mode
     08           Number of satellites being tracked
     0.9          Horizontal dilution of position
     545.4,M      Altitude, Meters, above mean sea level
     46.9,M       Height of geoid (mean sea level) above WGS84
                      ellipsoid
     (empty field) time in seconds since last DGPS update
     (empty field) DGPS station ID number
     *47          the checksum data, always begins with *
*/
void parseGPGGA(const char *pBuffer) {
  float geoIDHeight, altitude, HDOP, latitudeDegrees, longitudeDegrees, latitude, longitude;
  char degreebuff[10];  
  char lat, lon, mag;
  uint8_t hour, minute, seconds, year, month, day, fixquality, satellites;
  uint16_t milliseconds;
  int32_t latitude_fixed, longitude_fixed, degree;
  long minutes;

  // Ignore time
  parseTime(&pBuffer);

  // Ignore latitude
  parseLatLong(&pBuffer, true);
  
  // Ignore longitude
  parseLatLong(&pBuffer, false);

  pBuffer = strchr(pBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != *pBuffer)
    fixquality = atoi(pBuffer);

  pBuffer = strchr(pBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != *pBuffer)
    satellites = atoi(pBuffer);

  pBuffer = strchr(pBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != *pBuffer)
    HDOP = atof(pBuffer);
  
  pBuffer = strchr(pBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != *pBuffer)
    altitude = atof(pBuffer);
    
  pBuffer = strchr(pBuffer, NMEA_DELIMITER) + 1;
  pBuffer = strchr(pBuffer, NMEA_DELIMITER) + 1;
  if (NMEA_DELIMITER != *pBuffer)
    geoIDHeight = atof(pBuffer);
}

//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: setupHC12()
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
// Function Name: loop
// Purpose: The main loop of execution
// Inputs: N/A
// Output: N/A
// Notes: 
//
void loop() {

  // Read the serial
  if (Serial.available()) {
   char c = Serial.read();
   GPS.write(c);
  }
  //handleHC12();
  //handleSerialBuffer();
  handleGPS();
}
