#include <WiFi.h>
#include <ArduinoMqttClient.h>
#include <ESP32Servo.h>
#include "Ultrasonic.h"
#include <esp_wifi.h>
#include <esp_now.h>

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
long ultrasonic_right_cm;
long ultrasonic_down_cm;

#define clientId "ESP32_Device"

RTC_DATA_ATTR int max_reset_count = 0;
 
constexpr char WIFI_SSID[] = "Scarlet&Gray2.4";

int output = 0;
int output2 = 1;
int output3 = 0;
int loop_counter = 0;
int auto_mode_flag = 0;

#define uS_TO_S_FACTOR 1500000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */

 
WiFiClient espClient;
MqttClient mqttClient(espClient);
Ultrasonic ultrasonic_right(actuator_fb);
Ultrasonic ultrasonic_down(actuator_sf);

esp_now_peer_info_t peerInfo;

typedef struct struct_message {
  int espnow_out1;
  int espnow_out2;
  int espnow_out3;
} struct_message;

uint8_t broadcastAddress[] = {0x78, 0xE3, 0x6D, 0x11, 0xA1, 0x04};

struct_message myData;
struct_message giveData;

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Int: ");
  Serial.println(myData.espnow_out1);
  output = myData.espnow_out1;
  Serial.print("Float: ");
  Serial.println(myData.espnow_out2);
  output2 = myData.espnow_out2;
  Serial.print("Bool: ");
  Serial.println(myData.espnow_out3);
  output3 = myData.espnow_out3;
  Serial.println();
}

void(* resetFunc) (void) = 0;//declare reset function at address 0

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

//AUTOMATION FUNCTIONS

void actuatorOff() {
    digitalWrite(actuator_in1, LOW);
    digitalWrite(actuator_in2, LOW);
    digitalWrite(actuator_en, LOW);
//    Serial.println("OFF ACTUATOR");
}

void controlActuator(char direc) {
  if (direc == 'e') {
    digitalWrite(actuator_in1, HIGH);
    digitalWrite(actuator_in2, LOW);
    digitalWrite(actuator_en, HIGH);
//    Serial.println("EXTEND ACTUATOR");
  }
  else if (direc == 'r') {
    digitalWrite(actuator_in1, LOW);
    digitalWrite(actuator_in2, HIGH);
    digitalWrite(actuator_en, HIGH);
//    Serial.println("RETRACT ACTUATOR");
  }
  delay(9000);
  actuatorOff();
}

void driveForward(int speed_percentage, int duration) {
  int converted_speed = (((speed_percentage)*63535)/100)+ 2000;
  digitalWrite(D1, LOW);
  ledcWrite(0, converted_speed);
  if (duration == 999) {
    return;
  }
  delay(duration);
  ledcWrite(0, 0);
}

void driveBackward(int speed_percentage, int duration) {
  int converted_speed = (((speed_percentage)*63535)/100)+ 2000;
  digitalWrite(D1, HIGH);
  ledcWrite(0, converted_speed);
  if (duration == 999) {
    return;
  }
  delay(duration);
  ledcWrite(0, 0);
}

void turnRight(int speed_percentage, int duration) {
  int converted_speed = (((speed_percentage)*63535)/100)+ 2000;
  digitalWrite(D2, HIGH);
  ledcWrite(1, converted_speed);
  if (duration == 999) {
    return;
  }
  delay(duration);
  ledcWrite(1, 0);
}

void turnLeft(int speed_percentage, int duration) {
  int converted_speed = (((speed_percentage)*63535)/100)+ 2000;
  digitalWrite(D2, LOW);
  ledcWrite(1, converted_speed);
  if (duration == 999) {
    return;
  }
  delay(duration);
  ledcWrite(1, 0);
}

void upStairs() { // START WITH VEHICLE ANGLED ON BOTTOM STAIR
  for (int i = 0; i <= 15; i++) {
    controlActuator('e');
    delay(20);
    driveBackward(40,900);
    delay(20);
    controlActuator('r');
    delay(20);
    driveBackward(30,300);
    delay(20);
  }
  controlActuator('e');
  delay(20);
  driveBackward(40,200);
  driveBackward(30,200);
  driveBackward(20,200);
  driveBackward(15,600);
  delay(20);
  controlActuator('r');
}

void forwardUntilStair(int power) {
    while (ultrasonic_down_cm < 18 || ultrasonic_down_cm == 527) {    //Stops when past the first stair
      ultrasonic_down_cm = ultrasonic_down.MeasureInCentimeters();
      delay(5);
      driveForward(power, 999);
    }
    driveForward(0, 0);
    delay(500);
}
void downStairs() { // START WITH VEHICLE ABOVE TOP STEP, WITH ACTUATOR FREE TO LOWER 
  for (int i = 0; i < 16; i++) {
//    forwardUntilStair(13);
    controlActuator('e');
    delay(20);
    driveForward(16,405);   //440
    delay(800);
    controlActuator('r');
    delay(20);
    driveForward(16,445);   //480
    delay(20);
    driveBackward(18, 1000);
    delay(20);
    turnLeft(40, 60);
    delay(20);
    driveBackward(18, 1000);
    delay(400);
//    forwardUntilStair(13);

    delay(800);
  }
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

//  pinMode(actuator_sf, INPUT);
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

  giveData.espnow_out1 = 0;
  giveData.espnow_out2 = 0;
  giveData.espnow_out3 = 0;  
  
  WiFi.mode(WIFI_STA);
  int32_t channel = getWiFiChannel(WIFI_SSID);
  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else {
    Serial.println("ESP now intialised");
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &giveData, sizeof(giveData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(100);
  
  if (output == 0) {
    PRINTLN("Entering Deep Sleep");
    Serial.print("Output: ");
    Serial.println(output);
    Serial.println("INFO: Closing the Wifi connection");
    WiFi.disconnect();
    esp_deep_sleep_start();
    delay(500);
    
  }
  Serial.println("Starting Main Program");
  digitalWrite(RELAY, HIGH);
  ledBlink(3, 200);
  
}
 
void loop() {
  loop_counter++;
  Serial.print(output);
  Serial.print(" ");
  Serial.print(output2);
  Serial.print(" ");
  Serial.print(output3);
  Serial.println(" ");
  ultrasonic_right_cm = ultrasonic_right.MeasureInCentimeters();
  ultrasonic_down_cm = ultrasonic_down.MeasureInCentimeters();
//  Serial.print("CM Right: ");
//  Serial.println(ultrasonic_right_cm);
//  Serial.print("CM Down: ");
//  Serial.println(ultrasonic_down_cm);

  
  if (output3 == 1 && auto_mode_flag == 0) {
                                                                //AUTO MODE
    auto_mode_flag = 1; //Prevents auto mode from repeating
    ledBlink(3, 500);
    delay(3000);

      driveForward(14, 10000);
      delay(6000);
      while (ultrasonic_right_cm < 20) {    //Stops when past the stair wall
        ultrasonic_right_cm = ultrasonic_right.MeasureInCentimeters();
        delay(5);
        driveForward(13, 999);
      }
      delay(1200);        //Block of code turns right, drives forward towards dog bed, turns left, then backs into the stairs
      driveForward(0, 0);
      delay(1000);
      turnRight(40, 1000);
      delay(1000);
      driveForward(13, 2600);
      delay(1000);
      turnLeft(40, 1000);
      delay(1000);
      driveBackward(13, 4000);
//
//
      driveBackward(60, 370);   //Angles up the stairs
      delay(600);
      upStairs();

      driveBackward(15, 3000);
      turnLeft(38, 1000);
      driveForward(15, 8000);
      delay(8000);
      driveBackward(15, 9500);
      turnRight(38, 1100);
      delay(1000);
      while (ultrasonic_down_cm < 18 || ultrasonic_down_cm == 527) {    //Stops when past the first stair
        ultrasonic_down_cm = ultrasonic_down.MeasureInCentimeters();
        delay(5);
        driveForward(13, 999);
      }    
      driveForward(0, 0);

      delay(500);
      controlActuator('e');
      driveForward(15,300);
      delay(800);
      controlActuator('r');
      driveForward(15,460);
      driveBackward(18, 1000);
      downStairs();


      driveForward(15, 1500);
      delay(500);
      turnRight(40, 1000);
      delay(500);
      driveForward(13, 4000);
      delay(500);
      driveBackward(13, 6500);
      delay(500);
      turnLeft(40, 950);
      delay(500);
      driveBackward(15, 10000);
    driveBackward(15, 2000);    //Backs into the charging plate, turning to stay leaning on the wall
    turnLeft(30, 200);
    driveBackward(15, 2000);
    turnLeft(30, 200);
    driveBackward(15, 2000);
    turnLeft(30, 200);
    driveBackward(13, 6000);
    
      
    //END AUTO MODE
    ledBlink(3, 500);
  }

  if (output3 == 0 && auto_mode_flag == 1) {
    //Resets ability to go into auto mode after switching MQTT back to normal
    auto_mode_flag = 0;
  }

  int dig_1 = 0;
  int dig_2 = 0;
//  Serial.print("Output: ");
//  Serial.println(output);
  if (loop_counter % 50 == 0) {
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &giveData, sizeof(giveData));
  }
  
  if (output == 0) {
    Serial.println("Entering Deep Sleep");
    digitalWrite(RELAY, LOW);
    ledBlink(2, 100);
    WiFi.disconnect();
    delay(100);
//    Serial.println("Starting Sleep Now");
    esp_deep_sleep_start();
    delay(500);
  } 
  else {
    digitalWrite(RELAY, HIGH);
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

  if (output2 == 2) {

    digitalWrite(actuator_in1, HIGH);
    digitalWrite(actuator_in2, LOW);
    digitalWrite(actuator_en, HIGH);
//    Serial.println("EXTEND ACTUATOR");
  }
  else if (output2 == 1) {
    // OFF ACTUATOR
    digitalWrite(actuator_in1, LOW);
    digitalWrite(actuator_in2, LOW);
    digitalWrite(actuator_en, LOW);
//    Serial.println("OFF ACTUATOR");
  }
  else if (output2 == 0) {

    digitalWrite(actuator_in1, LOW);
    digitalWrite(actuator_in2, HIGH);
    digitalWrite(actuator_en, HIGH);
//    Serial.println("RETRACT ACTUATOR");
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
//  Serial.println(loop_counter);
  
//  delay(20);
}
