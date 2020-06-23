/*********
  - ESP-WROOM-32 Bluetooth and WIFI Dual Core CPU with Low Power Consumption
    https://www.aliexpress.com/item/32864722159.html?spm=a2g0s.9042311.0.0.7ac14c4dv0nB1k
  - SN65HVD230 CAN bus transceiver
    https://www.aliexpress.com/item/32686393467.html?spm=a2g0s.9042311.0.0.7ac14c4dv0nB1k
   
  by © SEKOM.com - Dr. András Szép 2020 using open source libraries v1.0
==================================================================================
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
===================================================================================

  
  CAN bus connections TX2 - GPIO17
                      RX2 - GPIO16 

  LED 5 on when WIFI is ON
  PIN 19 is the SWITCH to turn ON/OFF WiFi.
               
 
********/
#ifndef CONFIG_H
#define CONFIG_H

/*
 *
 * Note: default MSS for ESPAsyncTCP is 536 byte and defualt ACK timeout is 5s.
*/

// Replace with your network credentials 
const char* ssid = "your_ssid";  // Enter SSID here
const char* passworterminald = "your_password";  //Enter Password here
#define MYSSID "your_ssid"
#define MYPASSWORD "your_password"
#define SERVER_HOST_NAME "your_ip (numeric)"
#define TCP_PORT 2222
#define DNS_PORT 53


// CAN BUS
#define ESP32_CAN_TX_PIN GPIO_NUM_17  //TX2 Set CAN TX GPIO15 =  D8
#define ESP32_CAN_RX_PIN GPIO_NUM_16  //RX2 Set CAN RX GPIO13 =  D7

#define LED 5    // builtin LED GPIO05
//#define SWITCH 19   // ON/Off Switch for WIFI ~GPIO19
int buttonState = 0;         // variable for reading the pushbutton status


#endif // CONFIG_H
