#include "myConnectToWiFi.h"

#include <SLIPEncodedUSBSerial.h>
#include <WiFi.h>

  void myConnectToWiFi(char ssid[], char pass[]) {
      
   // bool isConnected = false;
  
    WiFi.begin(ssid, pass); // Enable WiFi and define Network Name + Password
    WiFi.setTxPower(WIFI_POWER_11dBm); // ...Power WiFI
  
  }
