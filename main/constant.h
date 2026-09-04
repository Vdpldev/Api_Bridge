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


// Define the structure to hold current device states
#define STORAGE_SIGNATURE 0xDEADBEEF
#define EEPROM_SIZE 512

struct TuyaDeviceState {
   uint32_t signature; // Should always be 0xDEADBEEF if data is valid
    // 1-4 Gang Switches
    bool switch_1;
    bool switch_2;
    bool switch_3;
    bool switch_4;

    // Fan Specifics
    bool fan_power;
    uint8_t fan_speed; // Usually 1-3 or 1-6
    uint8_t fan_mode;  // Nature, Sleep, etc.

    // System Metadata
    unsigned long last_updated; // Timestamp (millis)
    bool is_online;
};

// Create a global instance of the status
TuyaDeviceState currentStatus = {0xDEADBEEF,false, false, false, false, false, 0, 0, 0, false};

// Protocol fixed bytes
enum TuyaProtocol {
    TUYA_HEADER_HIGH = 0x55,
    TUYA_HEADER_LOW  = 0xAA,
    TUYA_VERSION_03  = 0x03
};

// Official Tuya Command IDs (The 4th byte in the frame)
enum TuyaCommand {
    TUYA_CMD_HEARTBEAT     = 0x00,
    TUYA_CMD_PRODUCT_INFO  = 0x01,
    TUYA_CMD_WORKING_MODE  = 0x02,
    TUYA_CMD_REPORT_STATUS = 0x07, // MCU reports state to Module
    TUYA_CMD_SEND_COMMAND  = 0x06, // Module sends command to MCU
    TUYA_CMD_QUERY_STATUS  = 0x08  // Module queries MCU
};

// Data Point IDs (DPIDs) - Specific to your device
enum TuyaDPID {
    DPID_SWITCH_1  = 0x01,
    DPID_FAN_SWITCH = 0x66, // 102: Fan On/Off
    DPID_FAN_SPEED  = 0x68  // 104: Fan Speed (1-3 or 1-6)
};
// Frame Markers
enum OemProtocol {
    OEM_START_BYTE = 0x7B, // '{' - Start of Frame
    OEM_END_BYTE   = 0x7D  // '}' - End of Frame
};

// Command types for your specific system
enum OemCommand {
    OEM_CMD_UPDATE        = 0x00,
    OEM_CMD_CONTROL       = 0xA2,
    OEM_CMD_ACK           = 0x06,
    OEM_CMD_ERROR         = 0x15
};
enum OemSTATUS {
    OEM_SWITCH_ON = 0x00,
    OEM_SWITCH_OFF= 0xFF
};
// Error Codes (Optional but helpful)
enum OemError {
    ERR_CHECKSUM = 0x01,
    ERR_INVALID_CMD = 0x02,
    //ERR_TIMEOUT = 0x03
};


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
DeviceState currentState = ST_OPERATIONAL;

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