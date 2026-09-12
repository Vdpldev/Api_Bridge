#include "constant.h"

// #define DEBUG
void pushToMsgQueue(const byte *frame, size_t len)
{
  String msgStr = "";
  msgStr.reserve(len);
  for (size_t i = 0; i < len; i++)
  {
    msgStr += (char)frame[i];
  }
  msgQueue.push_back(msgStr);
}

void setup()
{

  Serial.begin(DEBUG_BAUD);

#ifdef DEBUG
  Serial.println("--- SYSTEM BOOTING ---");
#endif
}

// Function to send Heartbeat (Command 0x00)
void maintainHeartbeat()
{

  if (millis() - lastHeartbeatTime >= 15000)
  {
    FrameHeartbeat();
    lastHeartbeatTime = millis();

#ifdef DEBUG
    Serial.println(F("[TUYA] Heartbeat Sent"));
#endif
  }
}

// Function to report WiFi Status to MCU (Command 0x03)
void reportWifiStatus()
{

  if (currentHeartBeatStatus != lastWifiStateReported)
  {
    lastWifiStateReported = currentHeartBeatStatus;

    byte f[8] = {0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, currentHeartBeatStatus, 0x00};

    // Calculate Checksum
    byte cs = 0;
    for (int i = 0; i < 7; i++)
      cs += f[i];
    f[7] = cs;

    sendFrame(f, sizeof(f), "HWifi Status");

#ifdef DEBUG
    Serial.print(F("[TUYA] WiFi Status Updated: "));
    Serial.println(currentStatus);
#endif
  }
}

// The updated Loop
void loop()
{

  runStateMachine(); // Your existing State Machine logic

  maintainHeartbeat(); // Handles 5s timer for Heartbeat

  reportWifiStatus(); // Reports WiFi/MQTT status changes
  if (!msgQueue.empty() && (millis() - lastSerialSend > 100))
  {

    String nextMsg = msgQueue.front();
    OemToTuya(&nextMsg);
    msgQueue.pop_front();
    Serial.print(nextMsg);
    lastSerialSend = millis();
  }
}