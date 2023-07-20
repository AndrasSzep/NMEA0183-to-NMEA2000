/*
 Demo: NMEA0183 library. NMEA0183 -> NMEA2000
   Reads messages from NMEA0183 and forwards them to the N2k bus
   Also forwards all NMEA2000 bus messages to the PC (Serial)

 This example reads NMEA0183 messages from one serial port. It is possible
 to add more serial ports for having NMEA0183 combiner functionality.

 The messages, which will be handled has been defined on NMEA0183Handlers.cpp
 on NMEA0183Handlers variable initialization. So this does not automatically
 handle all NMEA0183 messages. If there is no handler for some message you need,
 you have to write handler for it and add it to the NMEA0183Handlers variable
 initialization. If you write new handlers, please after testing send them to me,
 so I can add them for others use.
*/
#define LITTLE_FS 0
#define SPIFFS_FS 1
#define FILESYSTYPE SPIFFS_FS
// const char *otahost = "OTASPIFFS";
#define FILEBUFSIZ 4096

#ifndef WM_PORTALTIMEOUT
#define WM_PORTALTIMEOUT 180
#endif
#define N2K_SOURCE 15

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFiServer.h>
#include <NTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include "config.h"
#include "mywifi.h"
#include "BoatData.h"
#include "aux_functions.h"
#include <math.h>
#include <NMEA2000_CAN.h>
#include <N2kMessages.h>
#include <N2kMessagesEnumToStr.h>
#include <time.h>
#include <N2kMsg.h>
#include <NMEA2000.h>
//#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>
//#include "NMEA0183Handlers.h"

#define NMEA0183SourceGPSCompass 3
#define NMEA0183SourceGPS 1

unsigned long previousMillis = 0;       // Variable to store the previous time
unsigned long storedMillis = 0;         // time of last stored env.data
unsigned long previousPacketMillis = 0; // Variable to store the previous packet reception time

char nmeaLine[MAX_NMEA0183_MSG_BUF_LEN]; // NMEA0183 message buffer
size_t i = 0, j = 1;                     // indexers
uint8_t *pointer_to_int;                 // pointer to void *data (!)
int noOfFields = MAX_NMEA_FIELDS;        // max number of NMEA0183 fields
String nmeaStringData[MAX_NMEA_FIELDS + 1];

WiFiUDP udp;
NTPClient timeClient(udp, ntpServer, timeZone);
time_t utcTime = 0;
//sBoatData stringBD; // Strings with BoatData
tBoatData BoatData;

#ifdef OTAPORT
WebServer servOTA(OTAPORT); // webserver for OnTheAir update on port 8080
bool fsFound = false;
#include "webpages.h"
#include "filecode.h"
#endif

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n**** NMEA0183 (udp) ->-> N2K ****\n");
  delay(100);
#ifdef ENVSENSOR
//    M5.begin();             // Init M5StickC.  初始化M5StickC
    Wire.begin();  // Wire init, adding the I2C bus.  Wire初始化, 加入i2c总线
    qmp6988.init();
  Serial.println(F("ENVIII Hat(SHT30 and QMP6988) has initialized "));
#endif

  initFS(false,false); // initialalize file system SPIFFS

  initWiFi();           // init WifI from SPIFFS or WPS
  // Initialize the NTP client
  timeClient.begin();
//  time_t timestamp = time(nullptr); // store UTC
  
#ifdef UDPPort
  // Initialize UDP
  udp.begin(UDPPort);
  Serial.print("UDP listening on: ");
  Serial.println(UDPPort);
#endif

 // use mdns for host name resolution
  uint64_t chipID = ESP.getEfuseMac();
  char sn[30];                // to store mDNS name
  sprintf(sn, "%s%llu", servername, chipID);

  if (!MDNS.begin(sn)) // http://nmeaXXXXXXX.local
    Serial.println("Error setting up MDNS responder!");
  else
    Serial.print("mDNS responder started, OTA: http://");
  Serial.print(sn);

#ifdef OTAPORT
  Serial.printf(".local:%d\n", OTAPORT);
  servOTA.on("/", HTTP_GET, handleMain);

  // upload file to FS. Three callbacks
  servOTA.on(
      "/update", HTTP_POST, []()
      {
    servOTA.sendHeader("Connection", "close");
    servOTA.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); },
      []()
      {
        HTTPUpload &upload = servOTA.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // start with max available size
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          // flashing firmware to ESP
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          { // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });

  servOTA.on("/delete", HTTP_GET, handleFileDelete);
  servOTA.on("/main", HTTP_GET, handleMain); // JSON format used by /edit
  // second callback handles file uploads at that location
  servOTA.on(
      "/edit", HTTP_POST, []()
      { servOTA.send(200, "text/html", "<meta http-equiv='refresh' content='1;url=/main'>File uploaded. <a href=/main>Back to list</a>"); },
      handleFileUpload);
  servOTA.onNotFound([]()
                     {if(!handleFileRead(servOTA.uri())) servOTA.send(404, "text/plain", "404 FileNotFound"); });

  servOTA.begin();
#endif

  // Setup NMEA2000 system
  NMEA2000.SetProductInformation("19570428", // Manufacturer's Model serial code
                                 237, // Manufacturer's product code
                                 "NMEA0183 -> N2k",  // Manufacturer's Model ID
                                 "1.0 (2023-07-08)",  // Manufacturer's Software version code
                                 "1.0 (2023-07-08)" // Manufacturer's Model version
                                 );
  // Det device information
  NMEA2000.SetDeviceInformation(chipID, // Unique number. Use e.g. Serial number.
                                135, // Device function=PC Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                25, // Device class=Inter/Intranetwork Device. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf                               
                               );

  // NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text. Leave uncommented for default Actisense format.
  NMEA2000.SetForwardSystemMessages(true);
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode,25);
  //NMEA2000.EnableForward(false);
  NMEA2000.Open();
}

void loop() 
{
  tN2kMsg N2kMsg;   //n2k messagebuff
#ifdef ENVSENSOR
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= ENVINTERVAL)
  {
    pres = qmp6988.calcPressure();
    if (sht30.get() == 0) {  // Obtain the data of shT30.  获取sht30的数据
        tmp = sht30.cTemp;   // Store the temperature obtained from shT30.
        hum = sht30.humidity;  // Store the humidity obtained from the SHT30.
    } else {
        tmp = 0, hum = 0;
    }
        BoatData.Pressure = pres;
        BoatData.Humidity = hum;
        BoatData.AirTemperature = tmp + KToC;
        SetN2kOutsideEnvironmentalParameters(N2kMsg, 0, BoatData.WaterTemperature, BoatData.AirTemperature, BoatData.Pressure);
        NMEA2000.SendMsg(N2kMsg); 
        SetN2kHumidity(N2kMsg, 0,1, N2khs_InsideHumidity, BoatData.Humidity);
        NMEA2000.SendMsg(N2kMsg);
    if (currentMillis - storedMillis >= STOREINTERVAL)
    { // update stored value once/hour

      String Airtemps = updateStoredData("/temperature.txt", int(BoatData.AirTemperature));
      String Humidities = updateStoredData("/humidity.txt", int(BoatData.Humidity));
      String Pressures = updateStoredData("/pressure.txt", int(BoatData.Pressure));
      String Times = updateStoredData("/time.txt", time(nullptr));
      storedMillis = currentMillis;
    }
    previousMillis = currentMillis;
  }
#endif

  NMEA2000.ParseMessages();
//  NMEA0183_2.ParseMessages();
#ifdef UDPPort
  int packetSize = udp.parsePacket();
  if (packetSize) {
    processPacket(packetSize);
  }
#endif
#ifdef OTAPORT
  servOTA.handleClient();
#endif
//  ws.cleanupClients(); 
  SendSystemTime();
}

#define TimeUpdatePeriod 10000

void SendSystemTime() {
  static unsigned long TimeUpdated=millis();
  tN2kMsg N2kMsg;

  if ( (TimeUpdated+TimeUpdatePeriod<millis()) && BoatData.DaysSince1970>0 ) {
    SetN2kSystemTime(N2kMsg, 0, BoatData.DaysSince1970, BoatData.GPSTime);
    TimeUpdated=millis();
    NMEA2000.SendMsg(N2kMsg);
  }
}


void processPacket(int packetSize){
  char packetBuffer[4096];
  udp.read(packetBuffer, packetSize);
  packetBuffer[packetSize] = '\0';
#ifdef DEBUG
  Serial.println(packetBuffer);
#endif
  uint8_t *pointer_to_int;
  pointer_to_int = (uint8_t *)packetBuffer;
  if (packetSize == 1)
  { // in case just one byte was received for whatever reason
    nmeaLine[0] = pointer_to_int[0];
    j = 1;
  }
  else
  {
    for (i = 0; i < packetSize; i++)
    {
      nmeaLine[j++] = pointer_to_int[i];
      if (pointer_to_int[i - 1] == 13 && pointer_to_int[i] == 10)
      {
        nmeaLine[j] = 0;
        noOfFields = parseNMEA0183(nmeaLine, nmeaStringData);
        tN2kMsg N2kMsg;   //n2k messagebuff
        // act accodring to command
        String command = nmeaStringData[0];
        if (command == "APB")
        {
            //          Serial.print("APB");    //autopilot
        }
        else if (command == "DBK")
        {
            //          Serial.print("DBK");
        }
        else if (command == "DBT")  //#depth
        {
          BoatData.WaterDepth = nmeaStringData[3].toDouble();
          SetN2kWaterDepth(N2kMsg, 0, BoatData.WaterDepth, BoatData.Offset, 100.0);
          NMEA2000.SendMsg(N2kMsg); 
        }
        else if (command == "DPT")  //#depth
        {
          BoatData.WaterDepth = nmeaStringData[1].toDouble();
          BoatData.Offset = nmeaStringData[2].toDouble();
          SetN2kWaterDepth(N2kMsg, 0, BoatData.WaterDepth, BoatData.Offset, 100.0);
          NMEA2000.SendMsg(N2kMsg); 
        }
        else if (command == "GGA")      //#latlong
        {

          String UTC = nmeaStringData[1];
          int hours = UTC.substring(0, 2).toInt();
          int minutes = UTC.substring(2, 4).toInt();
          int seconds = UTC.substring(4, 6).toInt();
          BoatData.GPSTime = hours * 3600 + minutes * 60 + seconds;
          double coordinates = convertStringToGPS(nmeaStringData[2].toDouble());
          if ( nmeaStringData[3] == "S") { coordinates = -coordinates;}
          BoatData.Latitude = coordinates;
          coordinates = convertStringToGPS(nmeaStringData[4].toDouble());
          if ( nmeaStringData[5] == "W") { coordinates = -coordinates;}
          BoatData.Longitude = coordinates;
          BoatData.SatelliteCount = nmeaStringData[7].toInt();
          Serial.printf("%f %f\n", BoatData.Latitude, BoatData.Longitude);
          SetN2kGNSS(N2kMsg,0,BoatData.DaysSince1970,BoatData.GPSTime, BoatData.Latitude, BoatData.Longitude, BoatData.Altitude,
            N2kGNSSt_GPS,N2kGNSSm_noGNSS,BoatData.SatelliteCount,BoatData.HDOP,0,
            BoatData.GeoidalSeparation,1,N2kGNSSt_GPS,BoatData.DGPSReferenceStationID,BoatData.DGPSAge
            );
          NMEA2000.SendMsg(N2kMsg); 
        }
        else if (command == "GLL")
        {
          /*
          stringBD.Latitude = convertGPString(nmeaStringData[1]) + nmeaStringData[2];
          stringBD.Longitude = convertGPString(nmeaStringData[3]) + nmeaStringData[4];
          stringBD.UTC = nmeaStringData[5];
          int hours = stringBD.UTC.substring(0, 2).toInt();
          int minutes = stringBD.UTC.substring(2, 4).toInt();
          int seconds = stringBD.UTC.substring(4, 6).toInt();
          stringBD.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);
          */
          //          Serial.printf("GLL %s", timedate);
        }
        else if (command == "GSA")
        { // GPS Sat
            //
        }
        else if (command == "HDG")    //#heading
        {
          BoatData.MagHeading = nmeaStringData[1].toDouble()/radToDeg;
          BoatData.Deviation = nmeaStringData[2].toDouble()/radToDeg;
          BoatData.Variation = nmeaStringData[4].toDouble()/radToDeg;
          SetN2kMagneticHeading(N2kMsg, 1, BoatData.MagHeading, BoatData.Deviation, BoatData.Variation);
          NMEA2000.SendMsg(N2kMsg);
        }
        else if (command == "HDM")    //#heading
        {
          BoatData.MagHeading = nmeaStringData[1].toDouble()/radToDeg;
          SetN2kMagneticHeading(N2kMsg,1,BoatData.MagHeading,0,BoatData.Variation);
          NMEA2000.SendMsg(N2kMsg);
        }
        else if (command == "HDT")    //#heading
        {
          BoatData.TrueHeading = nmeaStringData[1].toDouble()/radToDeg;
          SetN2kTrueHeading(N2kMsg,1,BoatData.TrueHeading);
          NMEA2000.SendMsg(N2kMsg);
        }
        else if (command == "MTW")    //#watertemp
        {
          BoatData.WaterTemperature = nmeaStringData[1].toDouble()+KToC;
          SetN2kOutsideEnvironmentalParameters(N2kMsg, 0, BoatData.WaterTemperature, BoatData.AirTemperature, BoatData.Pressure);
          NMEA2000.SendMsg(N2kMsg); 
          SetN2kHumidity(N2kMsg, 0,1, N2khs_InsideHumidity, BoatData.Humidity);
          NMEA2000.SendMsg(N2kMsg);
        }
        else if (command == "MWD")    //#windspeed+angle
        {
          BoatData.WindDirectionT = nmeaStringData[1].toDouble()/radToDeg;
          BoatData.WindDirectionM = nmeaStringData[3].toDouble()/radToDeg;
          BoatData.WindSpeedK = nmeaStringData[5].toDouble();
          BoatData.WindSpeedM = nmeaStringData[7].toDouble();
          SetN2kWindSpeed(N2kMsg, 0,  BoatData.WindSpeedM,  BoatData.WindDirectionT, N2kWind_True_North );
          NMEA2000.SendMsg(N2kMsg);
        }
        else if (command == "MWV")
        { // wind speed and angle
        /*
            stringBD.WindDirectionT = String(int(nmeaStringData[1].toDouble()));

            stringBD.WindSpeedK = nmeaStringData[3];
*/
        }
        else if (command == "RMB")
        { // nav info
            //          Serial.print("RMB");        //waypoint info
        }
        else if (command == "RMC")
        { // nav info
        /*
            stringBD.SOG = nmeaStringData[7];

            stringBD.Latitude = convertGPString(nmeaStringData[3]) + nmeaStringData[4];

            stringBD.Longitude = convertGPString(nmeaStringData[5]) + nmeaStringData[6];

            stringBD.UTC = nmeaStringData[1];
            int hours = stringBD.UTC.substring(0, 2).toInt();
            int minutes = stringBD.UTC.substring(2, 4).toInt();
            int seconds = stringBD.UTC.substring(4, 6).toInt();
            stringBD.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);
            stringBD.Date = "20" + nmeaStringData[9].substring(4, 6) + "-" + nmeaStringData[9].substring(2, 4) + "-" + nmeaStringData[9].substring(0, 2);
*/
            //          Serial.printf("RMC %s", timedate);
        }
        else if (command == "RPM")  //#rpm
        { // engine RPM
          if (nmeaStringData[2] == "1")
          { // engine no.1
              BoatData.RPM = nmeaStringData[3].toDouble();
            SetN2kEngineParamRapid(N2kMsg, 0, BoatData.RPM, N2kDoubleNA,   0);        // PGN127488                 
            NMEA2000.SendMsg(N2kMsg);
          }
        }
        else if (command == "VBW")
        { // dual ground/water speed longitudal/transverse
            //
        }
        else if (command == "VDO")
        {
            //
        }
        else if (command == "VDM")
        {
            //
         }
        else if (command == "APB")
        {
          //
        }
        else if (command == "VHW")    // speed and Heading over water
        { 
          BoatData.SOW = nmeaStringData[5].toDouble()/msToKn;
          BoatData.TrueHeading = nmeaStringData[1].toDouble()/radToDeg;
          BoatData.MagHeading = nmeaStringData[3].toDouble()/radToDeg;
          SetN2kBoatSpeed(N2kMsg, 0, BoatData.SOW, BoatData.SOG, N2kSWRT_Paddle_wheel);
          NMEA2000.SendMsg(N2kMsg);
        }
        else if (command == "VTG")    // COG SOG
        {
          BoatData.COG = nmeaStringData[1].toDouble()/radToDeg;
          BoatData.SOG = nmeaStringData[5].toDouble()/msToKn;
          SetN2kCOGSOGRapid(N2kMsg, 0,N2khr_true, BoatData.COG, BoatData.SOG);
          NMEA2000.SendMsg(N2kMsg);
        }
        else if (command == "ZDA")
        { // Date&Time
        /*
            stringBD.Date = nmeaStringData[4] + "-" + nmeaStringData[3] + "-" + nmeaStringData[2];
            stringBD.UTC = nmeaStringData[1];
            int hours = stringBD.UTC.substring(0, 2).toInt();
            int minutes = stringBD.UTC.substring(2, 4).toInt();
            int seconds = stringBD.UTC.substring(4, 6).toInt();
            stringBD.UTC = int2string(hours) + ":" + int2string(minutes) + ":" + int2string(seconds);
*/
            //          Serial.printf("ZDA %s", timedate);
        }
        else
        {
            Serial.println("unsupported NMEA0183 sentence");
        }
        j = 0;
      }
    }
  }
}
