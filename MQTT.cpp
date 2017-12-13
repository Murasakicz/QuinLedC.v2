/*
  MQTT.cpp - Library for MQTT comunication.
  Created by Murasaki, November 14, 2017.

  MQTT class is for handling mqtt comunication
  Created by gutting and rewriting ESPHelper by
    ItKindaWorks | github.com/ItKindaWorks
 
  Mainly Dependent on Metro class and Wifi connection management
  
  Released into the public domain.
*/

#include "MQTT.h"

//empy initializer 
MQTT::MQTT(){  }

void MQTT::begin(FS* fs, String deviceName)
{
  _fs = fs;
  load_config();

  _deviceName = deviceName;
  if (_config.ip[0] != 0){
    // add chack if data is there
    connect();

    if(_mqttCallbackSet){
      //Serial.println("call back called");
      client.setCallback(_mqttCallback);
    }

    resubscribe();

  }
  //Serial.println(_config.deviceName);
  //Serial.println(_config.ip);
  //Serial.println(_config.port);
  //Serial.println(_config.user);
  //Serial.println(_config.password);*/

  
}

//Function for connection to MQTT server(Simple switch for selecting right connection)
bool MQTT::connect(){  
  switch (_connectionType) {
  case Unsecured:
    /* Blok 1 */   
    client = PubSubClient(_config.ip, _config.port, wifiClient);
    client.connect(_deviceName.c_str());
    if (client.connected()) {
      //Serial.println("MQTT Unsecured connected");
      return true;
      }
    return false;
    break;
  case SecureUser:
    /* Blok 2 */    
    client = PubSubClient(_config.ip, _config.port, wifiClient);
    client.connect(_deviceName.c_str(),_config.user.c_str(),_config.password.c_str());
    if (client.connected()) {
      //Serial.println("MQTT SecureUser connected");
      return true;
      }
    return false;
    break;
  case SecureTsl:
    /* Blok 3 */    
    client = PubSubClient(_config.ip, _config.port, wifiClientSecure);
    client.connect(_deviceName.c_str());
    if (client.connected()) {
      //Serial.println("MQTT SecureTsl connected");
      return true;
      }
    return false;
    // need function for cert handshake
    break;
  case SecureTslAndUser:
    /* Blok 4 */
    client = PubSubClient(_config.ip, _config.port, wifiClientSecure);
    client.connect(_deviceName.c_str(),_config.user.c_str(),_config.password.c_str());
    if (client.connected()) {
      //Serial.println("MQTT SecureTslAndUser connected");
      return true;
      }
    return false;
    break;
  default:
    /* Blok default */
    return false;
    break;
  }
}

//subscribe to a speicifc topic (does not add to topic list)
  //true on: subscription success
  //false on: subscription failed (either from PubSub lib or network is disconnected)
bool MQTT::subscribe(const char* topic, int qos){    
  if(client.connected()){
    bool returnVal = client.subscribe(topic, qos);

    //loop mqtt client
    client.loop();
    return returnVal;
  }

  else{return false;}
}

//add a topic to the list of subscriptions and attempt to subscribe to the topic on the spot
  //true on: subscription added to list (does not guarantee that the topic was subscribed to, only that it was added to the list)
  //false on: subscription not added to list
bool MQTT::addSubscription(const char* topic){ 
  //default return value is false
  bool subscribed = false;

  //loop throough finding the next available slot for a subscription and add it
  for(int i = 0; i < MAX_SUBSCRIPTIONS; i++){
    if(_subscriptions[i].isUsed == false){
      _subscriptions[i].topic = topic;
      _subscriptions[i].isUsed = true;
      subscribed = true;
      break;
    }
  }

  //if added to the list, subscibe to the topic
  if(subscribed){subscribe(topic, _qos);}
  
  return subscribed;
}

//loops through list of subscriptions and attempts to subscribe to all topics
void MQTT::resubscribe(){    
  //Serial.println("resubscribe to:");
  for(int i = 0; i < MAX_SUBSCRIPTIONS; i++){
    if(_subscriptions[i].isUsed){
      //Serial.println(_subscriptions[i].topic);
      subscribe(_subscriptions[i].topic, _qos);
      yield();
    }
  }
}

//attempts to remove a topic from the topic list
  //true on: subscription removed from list (does not guarantee that the topic was unsubscribed from, only that it was removed from the list)
  //false on: topic was not found in list and therefore cannot be removed
bool MQTT::removeSubscription(const char* topic){  
  bool returnVal = false;
  String topicStr = topic;

  //loop through all subscriptions
  for(int i = 0; i < MAX_SUBSCRIPTIONS; i++){
    //if an element is used, check for it being the one we want to remove
    if(_subscriptions[i].isUsed){
      String subStr = _subscriptions[i].topic;
      if(subStr.equals(topicStr)){
        //reset the used flag to false
        _subscriptions[i].isUsed = false;

        //unsubscribe
        client.unsubscribe(_subscriptions[i].topic);
        returnVal = true;
        break;
      }
    }
  }

  return returnVal;
}

//manually unsubscribes from a topic (This is basically just a wrapper for the pubsubclient function)
bool MQTT::unsubscribe(const char* topic){
  return client.unsubscribe(topic);
}

//publish to a specified topic
void MQTT::publish(const char* topic, const char* payload){    
  publish(topic, payload, false);
  //Serial.println("publish");
}

//publish to a specified topic with a given retain level
void MQTT::publish(const char* topic, const char* payload, bool retain){   
  client.publish(topic, payload, retain);
}

//set the callback function for MQTT
void MQTT::setMQTTCallback(MQTT_CALLBACK_SIGNATURE){
  _mqttCallback = callback;

  //only set the callback if using mqtt AND the system has already been started. Otherwise just save it for later 
  if(client.connected()) { // posible probleme
    client.setCallback(_mqttCallback);
    
    //Serial.println("set callback");
  }else{
    //Serial.println("set callback fail");
  }
    _mqttCallbackSet = true;

}

//legacy funtion - here for compatibility. Sets the callback function for MQTT (see function above)
bool MQTT::setCallback(MQTT_CALLBACK_SIGNATURE){
  setMQTTCallback(callback);
  //Serial.println("set callback");
  return true;
}

//main loop - should be called as often as possible - handles wifi/mqtt connection and mqtt handler
  //true on: network/server connected
  //false on: network or server disconnected
int MQTT::loop(){  
  if(WiFi.status() == WL_CONNECTED){
    //Serial.println("Wifi connected");
    //check for good connections and attempt a reconnect if needed

    if (!client.connected() && _config.ip[0] != 0) {
      //Serial.println("MQTT down... reconecting");
      reconnect();
    }
    
    //run the MQTT loop if we have a full connection
    if(client.connected()){client.loop();}
      
    return 0;
  }

  //return -1 for no connection because of bad network info
  return -1;
}

void MQTT::reconnect() {   
  
  // make sure we are connected to WIFI before attemping to reconnect to MQTT
  //----note---- maybe want to reset tryCount whenever we succeed at getting wifi connection?
  if(WiFi.status() == WL_CONNECTED){
    

      //attempt to connect to mqtt when we finally get connected to WiFi
      if(_config.ip[0] != 0){

      static int timeout = 0; //allow a max of 5 mqtt connection attempts before timing out
      if (!client.connected() && timeout < 5) {
        //debugPrint("Attemping MQTT connection");
          
        int connected = 0;

        connected = connect();

        //if connected, subscribe to the topic(s) we want to be notified about
        if (connected) {
          //debugPrintln(" -- Connected");

          resubscribe();
          timeout = 0;
        }
        else{
          //debugPrintln(" -- Failed");
        }
        timeout++;
       }

       //if we still cant connect to mqtt after 10 attempts increment the try count
       if(timeout >= 5 && !client.connected()){  
         timeout = 0;
         tryCount++;
       }
    }
  }
}


// Return MQTT connection state (This is basically just a wrapper for the pubsubclient function)
int MQTT::state(){
  return client.state();
}

String MQTT::constructChanelString(int channel, SubChanelType sub){
  String tmp = _deviceName;
         tmp += "/";
  
  if (channel == 1) { tmp += _config.channel1Id;}
  if (channel == 2) { tmp += _config.channel2Id;}

  tmp += "/";
  switch(sub){
    case Switch:
      tmp += _config.channelSwitchSubId;
      break;
    case Status:
      tmp += _config.channelStatusSubId;
      break;
    case Brightnes:
      tmp += _config.channelBrightnesSubId;
      break;
    case StatusBrightnes:
      tmp += _config.channelStatusBrightnesSubId;
      break;
    default:
      //tmp += _config.channel2Id;
      break;
    }

  //Serial.println(tmp);
  return tmp;  
}

bool MQTT::load_config() {
    File configFile = _fs->open(MQTT_CONFIG_FILE, "r");
    if (!configFile) {
        //Serial.println("Failed to open config file");
        return false;
    }

    size_t size = configFile.size();
    /*if (size > 1024) {
        DEBUGLOG("Config file size is too large");
        configFile.close();
        return false;
    }*/

    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    // We don't use String here because ArduinoJson library requires the input
    // buffer to be mutable. If you don't use ArduinoJson, you may as well
    // use configFile.readString instead.
    configFile.readBytes(buf.get(), size);
    configFile.close();
    //Serial.println("191 JSON file size: %d bytes\r\n", size);
    DynamicJsonBuffer jsonBuffer(1024);
    //StaticJsonBuffer<1024> jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(buf.get());

    if (!json.success()) {
        //Serial.println("Failed to parse config file\r\n");
        return false;
    }
/*#ifndef RELEASE
    String temp;
    json.prettyPrintTo(temp);
    Serial.println(temp);
#endif*/

    _config.ip = IPAddress(json["ip"][0], json["ip"][1], json["ip"][2], json["ip"][3]);
    _config.port = json["port"].as<int>();
    _config.user = json["user"].as<const char *>();
    _config.password = json["password"].as<const char *>();
    
    _config.tsl = json["tsl"].as<bool>();
    _config.firgerprint = json["firgerprint"].as<const char *>();

    _config.channel1Id = json["channel1Id"].as<const char *>();
    _config.channel2Id = json["channel2Id"].as<const char *>();
    _config.channelSwitchSubId = json["channelSwitchSubId"].as<const char *>();
    _config.channelStatusSubId = json["channelStatusSubId"].as<const char *>();
    _config.channelBrightnesSubId = json["channelBrightnesSubId"].as<const char *>();
    _config.channelStatusBrightnesSubId = json["channelStatusBrightnesSubId"].as<const char *>();

    if(!_config.tsl){
      if((_config.user[0] == '/0') && (_config.password[0] == '/0')){
        _connectionType = Unsecured;
      }else{
        _connectionType = SecureUser;
      }
    }else{
      if((_config.user[0] == '/0') && (_config.password[0] == '/0')){
        _connectionType = SecureTsl;
      }else{
        _connectionType = SecureTslAndUser;
      }
    }
    
    return true;
}


