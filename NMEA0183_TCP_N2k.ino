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

#include "config.h"

#define N2K_SOURCE 15f

#include <Arduino.h>
#include <Time.h>
#include <stdio.h>
#include <iostream>
// Import required libraries
#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
#else
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
  #include <Hash.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
#endif
#include <N2kMsg.h>
#include <NMEA2000.h>
#include <N2kMessages.h>
#include <NMEA0183.h>
#include <NMEA0183Msg.h>
#include <NMEA0183Messages.h>

// Set the information for other bus devices, which messages we support
const unsigned long TransmitMessages[] PROGMEM = {127250L, //Vessel heading
                                                  128259L,   // boat speed
                                                  128267L,   // water depth
                                                  129025L,   // lat/lon rapid
                                                  129026L,   // cog/sog rapid
                                                  129029L,   // GNSS
                                                  129033L,   // date/time
                                                  130306L, // Wind Data
                                                  130310L, // OutsideEnvironmental (obsolete!)
                                                  130311L, // EnvironmentalParameters (temperature/humidity/pressure)
                                                  130312L, // Temperature
                                                  130313L, // Humidity
                                                  130314L, // Pressure
                                                  130316L, // TemperatureExt
                                                  0
                                                 };
#include "NMEA0183Handlers.h"
#include "BoatData.h"

#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object

#define NMEA0183SourceGPSCompass 3
#define NMEA0183SourceGPS 1

tBoatData BoatData;

tNMEA0183 NMEA0183_3;
tNMEA0183 NMEA0183_TCP;

static ETSTimer intervalTimer;
char next_line[MAX_NMEA0183_MSG_BUF_LEN]; //NMEA0183 message buffer
size_t i=0, j=1;                          //indexers
uint8_t *pointer_to_int;                  //pointer to void *data (!)

static void replyToServer(void* arg) {
  AsyncClient* client = reinterpret_cast<AsyncClient*>(arg);

  // send reply
  if (client->space() > 32 && client->canSend()) {
    char message[32];
    sprintf(message, "this is from %s", WiFi.localIP().toString().c_str());
    client->add(message, strlen(message));
    client->send();
  }
}

/************************ event callbacks ***************************/
static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
//  Serial.printf("\n data received from %s \n", client->remoteIP().toString().c_str());
//  Serial.write((uint8_t*)data, len);
//  Serial.printf("%i: ", len);  
//  Serial.write((uint8_t*)data, len);

  uint8_t *pointer_to_int;
  pointer_to_int = (uint8_t *) data;
  
  if( len == 1) {                  //in case just one byte was received for whatever reason
    next_line[0] = pointer_to_int[0];
    j=1; 
//    Serial.printf("%c;", next_line[0]);
    }
  else {
    for ( i=0; i<len; i++) {
      next_line[j++] = pointer_to_int[i];
      if( pointer_to_int[i-1] == 13 && pointer_to_int[i] == 10 ) {
        next_line[j] = 0;
//        Serial.printf("%i: %s", j, next_line);    //here we got the full line ending CRLF
        NMEA2000.ParseMessages();
        NMEA0183_TCP.ParseTCPMessages( next_line,j);   //let's parse it     
        j=0;
      }
    }
  }  

    
  ets_timer_arm(&intervalTimer, 2000, true); // schedule for reply to server at next 2s
}

void onConnect(void* arg, AsyncClient* client) {
  Serial.printf("\n NMEA0183 listener has been connected to %s on port %d \n", SERVER_HOST_NAME, TCP_PORT);
  replyToServer(client);
}

//===============================================SETUP==============================
void setup() {
  Serial.begin(115200);  
  delay(20);
  Serial.println("\nNMEA0183 (TCP) to NMEA2000 converter v.1.0 by Dr.András Szép © 2020");
  // connects to access point
  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to %s:", MYSSID);
  WiFi.begin(MYSSID, MYPASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println(" connected!");
  AsyncClient* client = new AsyncClient;
  client->onData(&handleData, client);
  client->onConnect(&onConnect, client);
  client->connect(SERVER_HOST_NAME, TCP_PORT);

  ets_timer_disarm(&intervalTimer);
  ets_timer_setfn(&intervalTimer, &replyToServer, client);
  
  // Setup NMEA2000 system
  // Reserve enough buffer for sending all messages. This does not work on small memory devices like Uno or Mega
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  NMEA2000.SetProductInformation("00000008", // Manufacturer's Model serial code
                                 107, // Manufacturer's product code
                                 "NMEA0183 -> N2k -> PC",  // Manufacturer's Model ID
                                 "1.0.0.1 (2020-06-11)",  // Manufacturer's Software version code
                                 "1.0.0.0 (2020-06-11)" // Manufacturer's Model version
                                 );
  // Det device information
  NMEA2000.SetDeviceInformation(8, // Unique number. Use e.g. Serial number.
                                130, // Device function=PC Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                25, // Device class=Inter/Intranetwork Device. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20%26%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf                               
                               );

//  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text. Leave uncommented for default Actisense format.
  NMEA2000.SetForwardSystemMessages(true);
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode,32); // changed from 25
//  NMEA2000.ExtendTransmitMessages(TransmitMessages);
  //NMEA2000.EnableForward(false);
  Serial.println(" opening NMEA2000");
  NMEA2000.Open();
  Serial.println(" NMEA2000 Initialized");
  
  // Setup NMEA0183 ports and handlers
  InitNMEA0183Handlers(&NMEA2000, &BoatData);
//  NMEA0183_3.SetMsgHandler(HandleNMEA0183Msg);
  NMEA0183_TCP.SetMsgHandler(HandleNMEA0183Msg);
//  Serial3.begin(19200);
//  NMEA0183_3.SetMessageStream(&Serial3);
  NMEA0183_TCP.SetMessageStream(&Serial);   //just to have IsOpen() valid
//  NMEA0183_3.Open();
  NMEA0183_TCP.Open();
  Serial.println(" NMEA0183 Initialized");
}

void loop() {
/*  
  NMEA2000.ParseMessages();
  NMEA0183_TCP.ParseTCPMessages();
  */
  SendSystemTime();
/*
 * 
// Internal variables
tNMEA2000 *pNMEA2000=0;
tBoatData *pBD=0;   
      tN2kMsg N2kMsg;
      double MHeading=1.21;

      // Stupid Raymarine can not use true heading
      SetN2kMagneticHeading(N2kMsg,1,MHeading,0,0);
//      SetN2kPGNTrueHeading(N2kMsg,1,pBD->TrueHeading);
      pNMEA2000->SendMsg(N2kMsg);
       
 */
}

#define TimeUpdatePeriod 1000

void SendSystemTime() {
  static unsigned long TimeUpdated=millis();
  tN2kMsg N2kMsg;

  if ( (TimeUpdated+TimeUpdatePeriod<millis()) && BoatData.DaysSince1970>0 ) {
    SetN2kSystemTime(N2kMsg, 0, BoatData.DaysSince1970, BoatData.GPSTime);
    TimeUpdated=millis();
    NMEA2000.SendMsg(N2kMsg);
  }
}
