/* 
by Dr.András Szép under GNU General Public License (GPL).
*/
#include <Arduino.h>  //necessary for the String variables
#include <SPIFFS.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
//#include "config.h"
#include "mywifi.h"

int parseNMEA0183( String sentence, String data[]) {
  int noOfFields = 0;
  // Check if the sentence starts with a valid NMEA0183 header
  if (!sentence.startsWith("$")) {
    return -1;
  }

  // Find the asterisk marking the end of the sentence
  int endIdx = sentence.indexOf('*');
  if (endIdx == -1) {
    return -1;
  }

  // Check if the sentence length is sufficient for the checksum
  int sentenceLen = sentence.length();
  if (endIdx + 3 > sentenceLen) {
    return -1;
  }
  //count the number of fields
  int count = 0;
  for (size_t i = 0; i < sentence.length(); i++) {
    if (sentence.charAt(i) == ',') {
      count++;
    }
  }
  noOfFields = count;
  // Extract the data between the header and the asterisk
  String sentenceData = sentence.substring(3, endIdx+3);
  
  // Split the sentence data into fields using the comma delimiter
  int fieldIdx = 0;
  int startPos = 0;
  int commaIdx = sentenceData.indexOf(',', startPos);
  while (commaIdx != -1 && fieldIdx < noOfFields) {
    data[fieldIdx++] = sentenceData.substring(startPos, commaIdx);
    startPos = commaIdx + 1;
    commaIdx = sentenceData.indexOf(',', startPos);
  }
  // Add the remaining field (checksum) if there is room in the array
  data[fieldIdx] = sentence.substring(endIdx-1,endIdx);
  data[fieldIdx+1] = sentence.substring(endIdx+1,endIdx+3);
  return noOfFields+1;
}
/*
// Initialize SPIFFS
void initFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
   Serial.println("SPIFFS mounted successfully");
  }
}
*/
void storeString(String path, String content) {
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(content)) {
    Serial.printf("String %s stored at %s successfully\n", content.c_str(), path.c_str());
  } else {
    Serial.println("String store failed");
  }
  file.close();
}

String retrieveString(String path) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }
  String content = file.readString();
  file.close();
  Serial.printf("String %s from %s retrieved succesfully\n", content.c_str() , path.c_str());
  return content;
}

String int2string(int number) {
  if (number < 10) {
    return "0" + String(number);
  } else {
    return String(number);
  }
}

double convertStringToGPS(double coordinates){
//  Serial.print(coordinates);
  int deg = (int)(coordinates/100);
  double  min = coordinates - deg*100;
  coordinates = deg + min/60;
//  Serial.printf(": %d %f %f\n", deg, min, coordinates);
  return coordinates;
}

String convertGPString(String input) {
  // Extract degrees, minutes, and seconds from the input string
  int idx = input.indexOf(".");
  int degrees = input.substring(0, (idx-2)).toInt();
  int minutes = input.substring( (idx-2), idx).toInt();
  int seconds = int(input.substring( idx, idx+5).toFloat()*100.0*0.60);
  // Format the converted values into the desired format
  String output = int2string(degrees) + "º" + int2string(minutes) + "'" +  int2string(seconds) + "\"";
  return output;
}

String readStoredData( const char* filename){
  const int ARRAY_SIZE = 25;
  int iArray[ARRAY_SIZE];

  // Read data from the SPIFFS file
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.printf("Failed to open %s file\n", filename);
    return "";
  }

  // Read data into the array
  for (int i = 0; i < ARRAY_SIZE; i++) {
    iArray[i] = file.parseInt();
    if (file.peek() == ',') {
      file.read(); // Skip the comma
    }
  }
  file.close();

  // Generate comma-separated string from the array
  String commaSeparatedString;
  for (int i = 0; i < ARRAY_SIZE; i++) {
    commaSeparatedString += String(iArray[i]);
    if (i < ARRAY_SIZE - 1) {
      commaSeparatedString += ",";
    }
  }
  return commaSeparatedString;
}

String updateStoredData(const char* filename, int newValue) {
  const int ARRAY_SIZE = 25;
  int iArray[ARRAY_SIZE];

  // Read data from the SPIFFS file
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.printf("Failed to open %s file\n", filename);
    return "";
  }

  // Read data into the array
  for (int i = 0; i < ARRAY_SIZE; i++) {
    iArray[i] = file.parseInt();
    if (file.peek() == ',') {
      file.read(); // Skip the comma
    }
  }
  file.close();

  // Perform the required operations on the array
  for (int i = 0; i < ARRAY_SIZE - 1; i++) {
    iArray[i] = iArray[i + 1];
  }
  iArray[ARRAY_SIZE - 1] = newValue; // Replace with the new value

  // Write the modified data back to the SPIFFS file
  file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.printf("Failed to open %s file for writing\n", filename);
    return "";
  }

  // Write the modified array to the file
  for (int i = 0; i < ARRAY_SIZE; i++) {
    file.print(iArray[i]);
    if (i < ARRAY_SIZE - 1) {
      file.print(",");
    }
  }
  file.close();

  // Generate comma-separated string from the modified array
  String commaSeparatedString;
  for (int i = 0; i < ARRAY_SIZE; i++) {
    commaSeparatedString += String(iArray[i]);
    if (i < ARRAY_SIZE - 1) {
      commaSeparatedString += ",";
    }
  }
  #ifdef DEBUG
  Serial.printf("%s=", filename);
  Serial.println(commaSeparatedString);
  #endif
  return commaSeparatedString;
}

String getDT() {
  time_t now = time(nullptr);            // Get current time as time_t
  struct tm *currentTime = gmtime(&now); // Convert time_t to struct tm in UTC

  String dateTimeString;
  dateTimeString += String(currentTime->tm_year + 1900) + "-";
  dateTimeString += String(currentTime->tm_mon + 1) + "-";
  dateTimeString += String(currentTime->tm_mday) + " ";
  dateTimeString += String(currentTime->tm_hour) + ":";
  dateTimeString += String(currentTime->tm_min) + ":";
  dateTimeString += String(currentTime->tm_sec);

  return dateTimeString;
}

// Initialize WiFi
String  ssid, password;

void initWiFi() 
{
#ifdef STOREWIFI
  storeString("/ssid.txt", myssid);
  storeString("/password.txt", mypassword);
#endif
#ifdef READWIFI
  ssid = retrieveString(String("/ssid.txt"));
  password = retrieveString(String("/password.txt"));
#else
  ssid = myssid;
  password = mypassword;
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to WiFi ...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
}

