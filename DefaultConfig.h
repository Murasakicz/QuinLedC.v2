// -- Default config is used after flash -- //

const char* dc_ssid = "";
const char* dc_password = "";
int         dc_dhcp = 1;
IPAddress   dc_ip = IPAddress(192, 168, 100, 25);
IPAddress   dc_netmask = IPAddress(255, 255, 255, 0);
IPAddress   dc_gateway = IPAddress(192, 168, 100, 1);
IPAddress   dc_dns = IPAddress(192, 168, 100, 1);
const char* dc_ntpServerName = "pool.ntp.org";
int         dc_updateNTPTimeEvery = 15;
int         dc_timezone = 10;
int         dc_daylight = 1;
const char* dc_deviceName = "ESP8266fsa";
IPAddress   dc_mqttIp  = IPAddress(192, 168, 100, 20);
int         dc_mqttPort = 1883;
const char* dc_mqttUser = "";
const char* dc_mqttPassword = "";
