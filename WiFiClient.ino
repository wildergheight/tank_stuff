/*
    This sketch establishes a TCP connection to a "quote of the day" service.
    It sends a "hello" message, and then prints received data.
*/
#include <ArduinoJson.h>
#include <SPI.h>
#include <ESP8266WiFi.h>

#ifndef STASSID
#define STASSID "Scarlet&Gray"
#define STAPSK  "letsgoBucks"
#endif

String DEVID1 = "vB053531C1B839B1";
int status = WL_IDLE_STATUS;
int Liquid_level=0;
int level_flag=0;

boolean DEBUG = true;
const char* ssid     = STASSID;
const char* password = STAPSK;

const char* host = "djxmmx.net";
const uint16_t port = 17;

int pin_val = 0;

void setup() {
  Serial.begin(115200);

  // We start by connecting to a WiFi network
  pinMode(5,INPUT);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  Liquid_level=digitalRead(5);
  Serial.print("Liquid_level= ");
  Serial.println(Liquid_level,DEC);
  delay(5000);
  if ((Liquid_level < 1)&&(level_flag < 1)) {
    Serial.print("Sent");
//    String m = "Hours since first filled: " + h;
//    Serial.println(h);
    updatePushingBox(DEVID1);
    level_flag = 1;
   }
}

void updatePushingBox(String key){

String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "api.pushingbox.com";
  cmd += "\",80";
  ESP8266.print(cmd);
  Serial.print(cmd);
  delay(2000);
  if(ESP8266.find("Error")){
    Serial.println("Error posting data to pushingbox");
    return;
  }
  cmd = "GET /pushingbox?devid=" + key + "  HTTP/1.1\r\nHost: api.pushingbox.com\r\nUser-Agent: Arduino\r\nConnection: close\r\n\r\n";
   
  ESP8266.print("AT+CIPSEND=");
  ESP8266.print(cmd.length());
  delay(1000);
 
  ESP8266.print(cmd);
  //delay(1000);
 
  Get_reply(); 
  ESP8266.print("AT+CIPCLOSE");

}
