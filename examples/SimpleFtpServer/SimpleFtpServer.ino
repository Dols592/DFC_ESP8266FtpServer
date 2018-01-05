/*
  Small example of the DFC_ESP8266FtpServer. 
  
  This example accepts every username, and does not ask fo a password.
  set mServerUsername and mServerPassword to make it an username/password protected
  ftp server.
  
*/

#include "DFC_ESP8266FtpServer.h"
#include <ESP8266WiFi.h>

DFC_ESP7266FtpServer FtpServer;

const char* gWifiSid = "ssid";
const char* gWifiPassword = "password";

void setup(void)
{
  delay(1000); //Wait until esp is ready.
  Serial.begin(74880); //equals as default esp
  Serial.println();
  Serial.println("=============================================");
  Serial.println("Starting SimpleFtpServer");
  Serial.print("Mounting FS...");
  if (!SPIFFS.begin())
    Serial.println("SPIFFS Failed !!!");
  else
    Serial.println("SPIFFS Mounted");

  Serial.println("Initialisation of the ftp server");
  FtpServer.Init();

  Serial.print("Connecting to WiFi network...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(gWifiSid, gWifiPassword);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Initialisation Finished");
  Serial.println("=============================================");
}

void loop(void)
{
  FtpServer.Loop();
}
