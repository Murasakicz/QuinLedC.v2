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

int led1Value = 0, led2Value = 0;
int led1SetValue = 0, led2SetValue = 0;
uint32_t ledFadeTime = 0;


void ledFade() {
  if (millis() - ledFadeTime < 4ul) return;
  ledFadeTime = millis();

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

  analogWrite(PIN_LED1, led1Value);
  analogWrite(PIN_LED2, led2Value);
}

void callback(char* topic, uint8_t* payload, unsigned int length) {
  //logValue("Message arrived to topic: ", topic);
  
  Serial.println("Message arrived to topic");
  if (length > 4) {
    //logInfo("Message too long, ignored");
    return;
  }

  char valueRaw[5] = {0, 0, 0, 0, 0};
  strncpy(valueRaw, (char*)payload, length);
  int valueInt = String(valueRaw).toInt();

  

  bool topicLed1 = strstr(topic,MQTTServer._config.channel1Id.c_str()) != NULL;
  bool topicLed2 = strstr(topic,MQTTServer._config.channel1Id.c_str()) != NULL;
  bool topicSwitch = strstr(topic,MQTTServer._config.channelSwitchSubId.c_str()) != NULL;
  bool topicValue = strstr(topic,MQTTServer._config.channelBrightnesSubId.c_str()) != NULL;

  if (topicLed1) {
    if (topicValue) {
      //logValue("Setting value to LED1: ", valueInt);
      led1SetValue = valueInt;
    }

    if (topicSwitch) {
      if (valueInt) {
        if (led1SetValue > 0){
          //logInfo("Switch already on LED1");
        }else{
          led1SetValue = 1023;
          //logInfo("Switch on LED1");
        }
      } else {
        led1SetValue = 0;
        //logInfo("Switch off LED1");
      }
    }

    MQTTServer.publish(MQTTServer.constructChanelString(1, Status).c_str(), led1SetValue ? "1" : "0");
    MQTTServer.publish(MQTTServer.constructChanelString(1, StatusBrightnes).c_str(), String(led1SetValue).c_str());
  }


  if (topicLed2) {
    if (topicValue) {
      //logValue("Setting value to LED2: ", valueInt);
      led2SetValue = valueInt;
    }
    if (topicSwitch) {
      if (valueInt) {
        if (led2SetValue > 0){
          //logInfo("Switch already on LED2");
        }else{
          led2SetValue = 1023;
          //logInfo("Switch on LED2");
        }
      } else {
        led2SetValue = 0;
        //logInfo("Switch off LED2");
      }
    }
    MQTTServer.publish(MQTTServer.constructChanelString(2, Status).c_str(), led2SetValue ? "1" : "0");
    MQTTServer.publish(MQTTServer.constructChanelString(2, StatusBrightnes).c_str(), String(led2SetValue).c_str());
  }
  //logInfo("Message processing finished");
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

    MQTTServer.addSubscription(MQTTServer.constructChanelString(1, Brightnes).c_str());
    MQTTServer.addSubscription(MQTTServer.constructChanelString(2, Brightnes).c_str());
    MQTTServer.addSubscription(MQTTServer.constructChanelString(1, Switch).c_str());
    MQTTServer.addSubscription(MQTTServer.constructChanelString(2, Switch).c_str());
}

void loop() {
    /* add main program code here */
    MQTTServer.loop();  //run the loop() method as often as possible - this keeps the MQTT services running
    //server.handleClient();
    ledFade();
    
    // DO NOT REMOVE. Attend OTA update from Arduino IDE
    ESPHTTPServer.handle();	
}
