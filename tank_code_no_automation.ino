#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// Define Input Connections
#define CH1 22
#define D1 14
#define CH2 19
#define AN1 13
#define PRINT Serial.print
#define PRINTLN Serial.println
#define D2 27
#define AN2 12

#define RELAY 23

#define actuator_in1 5
#define actuator_in2 4
#define actuator_en 15
#define actuator_sf 21
#define actuator_fb 2

int ch1Value;
int ch1_direction;
int ch2Value;
int ch2_direction;

#define clientId "ESP32_Device"

RTC_DATA_ATTR int max_reset_count = 0;
 
const char* ssid = "Scarlet&Gray2.4";
const char* password =  "letsgoBucks";
const char* mqttServer = "192.168.1.118";
const int mqttPort = 1883;
byte output = 0;
byte output2 = 1;
int loop_counter = 0;

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */

 
WiFiClient espClient;
PubSubClient client(espClient);

 
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Entering Callback");
  if (strcmp(topic, "home-assistant/dylan/tank") == 0){
    
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");



  Serial.println(*payload);
  output = *payload;

  
  Serial.print("Output: ");
  Serial.println(output);
  Serial.println();
  Serial.println("-----------------------");
  
  }

  if (strcmp(topic, "home-assistant/dylan/tank/actuator") == 0){
    
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");



  Serial.println(*payload);
  output2 = *payload;

  
  Serial.print("Output2: ");
  Serial.println(output2);
  Serial.println();
  Serial.println("-----------------------");
  
  }
    
}

void ledBlink(int loops, int delay_duration) {
  for (int i = 0; i <= loops; i++) {
    digitalWrite(LED_BUILTIN, HIGH);  
    delay(delay_duration);                       
    digitalWrite(LED_BUILTIN, LOW);    
    delay(delay_duration);                      
  }
}

// Read the number of a specified channel and convert to the range provided.
// If the channel is off, return the default value
int readChannel(int channelInput, int minLimit, int maxLimit, int defaultValue){
  int ch = pulseIn(channelInput, HIGH, 30000);
  if (ch < 100) return defaultValue;
  return map(ch, 1000, 2000, minLimit, maxLimit);
}

// Read the switch channel and return a boolean value
bool readSwitch(byte channelInput, bool defaultValue){
  int intDefaultValue = (defaultValue)? 100: 0;
  int ch = readChannel(channelInput, 0, 100, intDefaultValue);
  return (ch > 50);
}

 
void setup() {
  int reset_count = 0;
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.begin(115200);
  pinMode(CH1, INPUT);
  pinMode(CH2, INPUT);
  
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(RELAY, OUTPUT);

  pinMode(actuator_in1, OUTPUT);
  pinMode(actuator_in2, OUTPUT);
  pinMode(actuator_en, OUTPUT);

  pinMode(actuator_sf, INPUT);
//  pinMode(actuator_fb, INPUT);    ANALOG PINS DONT NEED PINMODE
  
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(AN2, LOW);
  digitalWrite(RELAY, LOW);

  digitalWrite(actuator_in1, LOW);
  digitalWrite(actuator_in2, LOW);
  digitalWrite(actuator_en, LOW);
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  ledcSetup(0, 1000, 16);
  ledcAttachPin(AN1, 0);

  ledcSetup(1, 1000, 16);
  ledcAttachPin(AN2, 1);

  WiFi.begin(ssid, password);
  int wifi_timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    wifi_timeout = wifi_timeout + 1;
    delay(500);
    Serial.println("Connecting to WiFi..");
    if (wifi_timeout > 10) {
      Serial.println("WIFI TIMEOUT. RESTARTING");
      esp_deep_sleep_start();

    }
  }
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    
    if (client.connect("clientId.c_str()")) {
 
      Serial.println("connected");
      client.setKeepAlive(90);
      client.setSocketTimeout(90);
      client.setBufferSize(256);  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
 
  client.subscribe("home-assistant/dylan/tank", 1);
  client.subscribe("home-assistant/dylan/tank/actuator", 1);

  delay(50);

  for (int i = 0; i <= 20; i++) {
    if( !client.loop() ){
      //return some error
      Serial.println("SHIT BROKE");
      break;
    }
        delay(50);  // DO NOT DELETE

  }

  while (output == 0) {
    client.disconnect();
    Serial.println("Client Disconnected");
    delay(300);
    
    while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    
    if (client.connect(clientId)) {
 
      Serial.println("connected");
//      client.setKeepAlive(15);
//      client.setSocketTimeout(90);
      client.setBufferSize(256);  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
  client.subscribe("home-assistant/dylan/tank", 1);
  client.subscribe("home-assistant/dylan/tank/actuator", 1);

  for (int i = 0; i <= 20; i++) {
    if( !client.loop() ){
      //return some error
      Serial.println("SHIT BROKE");
      break;
    }
        delay(50);  // DO NOT DELETE

  }
  reset_count = reset_count + 1;
  }
  if (reset_count > max_reset_count) {
    max_reset_count = reset_count;
  }
  Serial.print("Max Needed Resets in one cycle: ");
  Serial.println(max_reset_count);
  
  delay(10);
  if (output == 48 || output == 0) {
    PRINTLN("Entering Deep Sleep");
    Serial.print("Output: ");
    Serial.println(output);
    Serial.println("INFO: Closing the MQTT connection");
    client.disconnect();
    Serial.println("INFO: Closing the Wifi connection");
    WiFi.disconnect();
    while (client.connected() || (WiFi.status() == WL_CONNECTED))

    {
      Serial.println("Waiting for shutdown before sleeping");
      delay(10);
    }
    delay(100);
    Serial.println("Starting Sleep Now");
    esp_deep_sleep_start();
    delay(500);
    
  }
  Serial.println("Starting Main Program");
  digitalWrite(RELAY, HIGH);
  ledBlink(3, 200);
  
}
 
void loop() {
  loop_counter++;
//  while (!client.connected()) {
//    ledBlink(3, 30);
//    Serial.println("Connecting to MQTT...");
//    
//    if (client.connect(clientId)) {
// 
//      Serial.println("connected");
//    } else {
// 
//      Serial.print("failed with state ");
//      Serial.print(client.state());
//      ledBlink(10, 50);
// 
//    }
//  }
  if (loop_counter %50 == 0) {
    Serial.println("MQTT DISCONNECTED");
    client.disconnect();
  }

  if (loop_counter %20 == 0 && !client.connected()) {
    Serial.println("CONNECTING MQTT");
    client.connect(clientId);
  }

  if (!client.connected()) {
//    Serial.print("AAAAAAAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH");
//    client.connect(clientId);
  }
  if (client.connected()) {
//    Serial.print("CONNNNNNNNECTEDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
    client.subscribe("home-assistant/dylan/tank", 1);
    client.subscribe("home-assistant/dylan/tank/actuator", 1);
  }
  Serial.println(client.connected());
  client.loop();
  int dig_1 = 0;
  int dig_2 = 0;
  Serial.print("Output: ");
  Serial.println(output);
  if (output == 48 || output == 0) {
    Serial.println("Entering Deep Sleep");
    digitalWrite(RELAY, LOW);
    ledBlink(2, 100);
    Serial.print("Output: ");
    Serial.println(output);
    Serial.println("INFO: Closing the MQTT connection");
    client.disconnect();
    Serial.println("INFO: Closing the Wifi connection");
    WiFi.disconnect();
    while (client.connected() || (WiFi.status() == WL_CONNECTED))

    {
      Serial.println("Waiting for shutdown before sleeping");
      delay(10);
    }
    delay(100);
    Serial.println("Starting Sleep Now");
    esp_deep_sleep_start();
    delay(500);
  } 
  ch1Value = readChannel(CH1, -90, 90, 0);
  ch2Value = readChannel(CH2, -90, 90, 0);
  //for actual motor setup
  //set range to -90 to 90
  //if ch1 value is less than 0
  //set dig_1 to 1 (or whatever negative is)
  //multiply ch1 by -1 (to set it to positive)
  //ch1 is analog 1
  
  if (ch1Value < 0) {
    dig_1 = 1;
    ch1Value = ch1Value *-1;
    digitalWrite(D1, HIGH);
  }
  else {
    dig_1 = 0;
    digitalWrite(D1, LOW);
  }

  if (ch2Value < 0) {
    dig_2 = 1;
    ch2Value = ch2Value *-1;
    digitalWrite(D2, HIGH);
  }
  else {
    dig_2 = 0;
    digitalWrite(D2, LOW);
  }
  
  int ch1Value_converted = (((ch1Value)*63535)/90)+ 2000;
  if (ch1Value_converted > 65535) {
    ch1Value_converted = 65535;
  }
  ledcWrite(0, ch1Value_converted);

  if (ch1Value < 5) {
    ch1Value_converted = 0;
  }

  int ch2Value_converted = (((ch2Value)*63535)/90)+ 2000;
  if (ch2Value_converted > 65535) {
    ch2Value_converted = 65535;
  }
  ledcWrite(1, ch2Value_converted);

  if (ch2Value < 5) {
    ch2Value_converted = 0;
  }

  if (output2 == 50) {
    // RETRACT ACTUATOR
    digitalWrite(actuator_in1, HIGH);
    digitalWrite(actuator_in2, LOW);
    digitalWrite(actuator_en, HIGH);
    Serial.println("EXTEND ACTUATOR");
  }
  else if (output2 == 49) {
    // OFF ACTUATOR
    digitalWrite(actuator_in1, LOW);
    digitalWrite(actuator_in2, LOW);
    digitalWrite(actuator_en, LOW);
    Serial.println("OFF ACTUATOR");
  }
  else if (output2 == 48) {
    // EXTEND ACTUATOR
    digitalWrite(actuator_in1, LOW);
    digitalWrite(actuator_in2, HIGH);
    digitalWrite(actuator_en, HIGH);
    Serial.println("RETRACT ACTUATOR");
  }
//  Serial.print("Ch1: ");
//  Serial.print(ch1Value_converted);
//  Serial.print("   % of Max: ");
//  double percentage = ((double)ch1Value_converted)/65535;
//  Serial.print(percentage, 2);
//  Serial.print(" Direction: ");
//  Serial.println(ch1Value);
//  Serial.print("  Actuator Value: ");
//  Serial.println(output2);
  Serial.println(loop_counter);

  delay(30);
}
