/*
  MQTT.h - Library for MQTT comunication.
  Created by Murasaki, November 14, 2017.

  MQTT class is for handling mqtt comunication
  Created by gutting and rewriting ESPHelper by
    ItKindaWorks | github.com/ItKindaWorks
 
  Mainly Dependent on Metro class and Wifi connection management
  
  Released into the public domain.
*/

#ifndef MQTT_h
#define MQTT_h

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define MQTT_CONFIG_FILE "/mqttconfig.json"
//Maximum number of subscriptions that can be auto-subscribed
//feel free to change this if you need more subsciptions
#define MAX_SUBSCRIPTIONS 10  

#define DEFAULT_QOS 1;  //at least once - devices are guarantee to get a message.

enum ConectionType { Dummy , Unsecured, SecureUser, SecureTsl, SecureTslAndUser };
enum SubChanelType { IN , OUT, BACKGROUND};

typedef struct {
    IPAddress  ip;
    int port;
    String user;
    String password;
    bool tsl;
    String firgerprint;
    
    String channel1IN;
    String channel1OUT;
    String channel2IN;
    String channel2OUT;
} strMQTTConfig;

struct subscription{
  bool isUsed = false;
  char* topic;
};
typedef struct subscription subscription;

class MQTT 
{
  public:
    MQTT();
    void begin(FS* fs, String deviceName);
    int loop();
    void end();
    int state();
    String constructChanelString(int channel,SubChanelType sub);
    strMQTTConfig _config;
    
    
    void setMQTTCallback(MQTT_CALLBACK_SIGNATURE);
    bool setCallback(MQTT_CALLBACK_SIGNATURE);
    bool addSubscription(const char* topic);    
    void publish(const char* topic, const char* payload);
    void publish(const char* topic, const char* payload, bool retain);
    
  private:
    int _pin;
    const char* mqttuser;
    const char* mqttpass;
    const char* mqtt_server_host;
  protected:
    String _deviceName;
    FS* _fs;
    ConectionType _connectionType;
    int tryCount = 0;

    PubSubClient client;
    WiFiClient wifiClient;
    WiFiClientSecure wifiClientSecure;
    const char* _fingerprint;
    bool _useSecureClient = false;

    #ifdef ESP8266
      std::function<void(char*, uint8_t*, unsigned int)> _mqttCallback;
    #endif
    #ifdef ESP32
      void(*_mqttCallback)(char*, uint8_t*, unsigned int) ;
    #endif
    bool _mqttCallbackSet = false;

    String _clientName;
    subscription _subscriptions[MAX_SUBSCRIPTIONS];
    int _qos = DEFAULT_QOS;

    bool load_config();
    String ipAdresss_to_string(IPAddress ip);
    bool connect();
    bool subscribe(const char* topic, int qos);
    void resubscribe();
    bool removeSubscription(const char* topic);
    bool unsubscribe(const char* topic);
    void reconnect();    
    
    //bool connect_normal();
    //bool connect_secure();
};


#endif
