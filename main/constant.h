#include <ESP8266WiFi.h>
#include <ArduinoJson.h> 
#include <queue> 
std::queue<String> msgQueue;

#ifndef CONSTANT_H
#define CONSTANT_H

IPAddress apIP(192, 168, 4, 1);
IPAddress apGateway(192, 168, 4, 1);
IPAddress apSubnet(255, 255, 255, 0);

const char* mqtt_username = "mqtt_mobile_client";
const char* mqtt_password = "pass123";

extern char mqtt_pub_topic[18]; 
extern char mqtt_sub_topic[18];

enum DeviceState 
{
  ST_INIT,
  ST_LOAD_CONFIG,
  ST_WIFI_CONNECT,
  ST_WIFI_WAITING,
  ST_MQTT_CONNECT,
  ST_OPERATIONAL,
  ST_OTA_CHECK,
  ST_OTA_PERFORM,
  ST_IDLE,
  ST_AP_MODE,
  ST_ERROR
};
DeviceState currentState = ST_INIT;

struct Config 
{
  char ssid[32];
  char password[32];
};

Config deviceSettings;

const byte RESET_FRAME[] = {0x7B, 0x54, 0x02, 0x02, 0x04, 0x7D};

#define CURRENT_VERSION "1.0.2"
#define OTA_URL "https://back.iotstudio.org/update_fw"

#define MQTT_BROKER "mbd.iotstudio.org"
#define MQTT_PORT 1883
#define MQTT_RETRY_MS 5000

#define TCP_BUFFER_SIZE 256
#define TCP_PORT 1234
WiFiServer server(TCP_PORT);

#define WIFI_TIMEOUT_MS 60000

#define EEPROM_SIZE 512
#define ADDR_SSID 0
#define ADDR_PASS 32

#define STATUS_LED LED_BUILTIN
#define DEBUG_BAUD 9600

#endif