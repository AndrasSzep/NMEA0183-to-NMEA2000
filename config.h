#include "N2kMessages.h"
/* 
by Dr.András Szép under GNU General Public License (GPL).
*/
#define DEBUG       //additional print of all data on serial
//#define STOREWIFI   // store wifi credentials on the SPIFFS
#define READWIFI    // get Wifi credentials from SPIFFS
#define ENVSENSOR       //environmental sensors connected

#define OTAPORT 8080    //OTA port  - if defined it means we can access the OTA interface to update files on the SPIFFs
const char*   servername  = "nmea";     //nDNS servername - http://servername.local
#define UDPPort 10110 // 10110 is the default NMEA0183 port# if defined it means we listen to the NMEA0183 messages broadcasted on this port
const char* ntpServer = "europe.pool.ntp.org";
const int timeZone = 0;  // Your time zone in hours
String  UTC ="2023-07-11 20:30:00";
#define TIMEPERIOD 60.0

#define MAX_NMEA0183_MSG_BUF_LEN 4096
#define MAX_NMEA_FIELDS  64
/*
#ifdef ENVSENSOR
#define SDA_PIN 26
#define SCL_PIN 32
#endif
*/
const double radToDeg = 180.0 / M_PI;
const double msToKn = 3600.0 / 1852.0;
const double kpaTommHg = 133.322387415;
const double KToC = 273.15;
//#define N2k_SPI_CS_PIN 53    // Pin for SPI select for mcp_can
//#define N2k_CAN_INT_PIN 21   // Interrupt pin for mcp_can
//#define USE_MCP_CAN_CLOCK_SET 8  // Uncomment this, if your mcp_can shield has 8MHz chrystal
#define ESP32_CAN_TX_PIN GPIO_NUM_22 // Uncomment this and set right CAN TX pin definition, if you use ESP32 and do not have TX on default IO 16
#define ESP32_CAN_RX_PIN GPIO_NUM_19 // Uncomment this and set right CAN RX pin definition, if you use ESP32 and do not have RX on default IO 4
//#define NMEA2000_ARDUINO_DUE_CAN_BUS tNMEA2000_due::CANDevice1    // Uncomment this, if you want to use CAN bus 1 instead of 0 for Arduino DUE

#ifdef ENVSENSOR
#include <M5StickC.h>
#include "M5_ENV.h"

SHT3X sht30;
QMP6988 qmp6988;
float tmp = 20.0;
float hum = 50.0;
float pres = 755.0;
#define ENVINTERVAL 10000     // Interval in milliseconds
#define STOREINTERVAL 60000 // store env data in SPIFFS ones/hour
#endif
#define UDPINTERVAL 1000 // ignore UDP received within 1 second
