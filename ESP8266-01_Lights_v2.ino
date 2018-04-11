#include <ESP8266WiFi.h>
#include "FS.h"
#include <WiFiClient.h>
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "FSWebServerLib.h"
#include "MQTT.h"
#include <Hash.h>

MQTT MQTTServer;

#define MQTT_ID "/QuinLed_2"
#define PIN_LED1 0 // Pin with 0
#define PIN_LED2 2 // Pin with 1

const int BUFFER_SIZE = JSON_OBJECT_SIZE(10);
#define MQTT_MAX_PACKET_SIZE 512

int led1Value = 0, led2Value = 0;
int led1SetValue = 0, led2SetValue = 0;
uint32_t ledFadeTime = 0; uint32_t transition = 0;
int chanel = 0;


void ledFade() {
  if (millis() - ledFadeTime < 4ul) return;
  ledFadeTime = millis()+( transition*1000);

  if (led1Value < led1SetValue) {
    led1Value++;
  }
  if (led1Value > led1SetValue) {
    led1Value--;
  }

  if (led2Value < led2SetValue) {
    led2Value++;
  }
  if (led2Value > led2SetValue) {
    led2Value--;
  }
  //Serial.println("Tick");
  analogWrite(PIN_LED1, led1Value);
  analogWrite(PIN_LED2, led2Value);
}

//***************** aurduinoJson extension 
bool containsNestedKey(const JsonObject& obj, const char* key) {
    for (const JsonPair& pair : obj) {
        if (!strcmp(pair.key, key))
            return true;

        if (containsNestedKey(pair.value.as<JsonObject>(), key)) 
            return true;
    }

    return false;
}

bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }

  if (chanel == 1){
    if (root.containsKey("state")){
      if(strcmp(root["state"], "ON") == 0){
        led1SetValue = 1023;
      }else if (strcmp(root["state"], "OFF") == 0) {
        led1SetValue = 0;
      }  
    }
    if (root.containsKey("brightness")) {
       led1SetValue = root["brightness"];
    }  
  }
  if (chanel == 2){
    if (root.containsKey("state")){
      if(strcmp(root["state"], "ON") == 0){
        led2SetValue = 1023;
      }else if (strcmp(root["state"], "OFF") == 0) {
        led2SetValue = 0;
      }  
    }
    if (root.containsKey("brightness")) {
       led2SetValue = root["brightness"];
    }  
  }
  if (root.containsKey("transitions")){
    transition = root["transitions"];  
  }
  return true;
}

void callback(char* topic, uint8_t* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  if (strstr(topic,MQTTServer.constructChanelString(1,IN).c_str()) != NULL){
    chanel = 1;
  }
  if (strstr(topic,MQTTServer.constructChanelString(2,IN).c_str()) != NULL){
    chanel = 2;
  }

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJson(message)) {
    return;
  }
  sendState();  
}

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  if (chanel == 1){
    if (led1SetValue > 0 ){
      root["state"] = "ON";  
    }else{
      root["state"] = "OFF";
    }  
    root["brightness"] = led1SetValue;
  }

  if (chanel == 2){
    if (led2SetValue > 0 ){
      root["state"] = "ON";  
    }else{
      root["state"] = "OFF";
    }  
    root["brightness"] = led2SetValue;
  }
  root["transitions"] = transition;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

   MQTTServer.publish(MQTTServer.constructChanelString(chanel,OUT).c_str(), buffer, true);
}

void setup() {
    // WiFi is started inside library
    SPIFFS.begin(); // Not really needed, checked inside library and started if needed
    ESPHTTPServer.begin(&SPIFFS);
    /* add setup code here */

    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
    analogWrite(PIN_LED1, 0);
    analogWrite(PIN_LED2, 0);
    //const char* test =  "/Led1/value"; 

    MQTTServer.setMQTTCallback(callback);
    MQTTServer.begin(&SPIFFS, ESPHTTPServer.getDeviceName());

    MQTTServer.addSubscription(MQTTServer.constructChanelString(1,IN).c_str());
    MQTTServer.addSubscription(MQTTServer.constructChanelString(2,IN).c_str());
    
    chanel = 1;
    sendState();
    chanel = 2;
    sendState();
}

void loop() {
    /* add main program code here */
    MQTTServer.loop();  //run the loop() method as often as possible - this keeps the MQTT services running
    //server.handleClient();
    ESPHTTPServer.mqttConnectionStatus = MQTTServer.state();
    ledFade();
    
    // DO NOT REMOVE. Attend OTA update from Arduino IDE
    ESPHTTPServer.handle();	
}
