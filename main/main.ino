#include "constant.h"

// #define DEBUG
void pushToMsgQueue(const byte* frame, size_t len) {
    String msgStr = "";
    msgStr.reserve(len); 
    for (size_t i = 0; i < len; i++) {
        msgStr += (char)frame[i];
    }
    msgQueue.push_back(msgStr);
}
void setup(){

  Serial.begin(DEBUG_BAUD);
  
  #ifdef DEBUG
    Serial.println("--- SYSTEM BOOTING ---");
  #endif

  // // 1. Frame: Heartbeat (Standard Tuya Check-in)
  // // 55 AA 00 00 00 00 FF
  // const byte heartbeatFrame[] = {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0xFF};
  
  // // 2. Frame: Query All Status (Forces MCU to report all DPIDs)
  // // 55 AA 00 08 00 00 07
  // const byte queryStatusFrame[] =  {0x55, 0xAA, 0x00, 0x08, 0x00, 0x00, 0x07};
  // pushToMsgQueue(queryStatusFrame, sizeof(queryStatusFrame));
  // const byte queryFrame[] = {0x7B, 0x02, 0x01, 0x01, 0x7D};
  // pushToMsgQueue(queryFrame, sizeof(queryFrame));
  // // // Push Heartbeat first
  // //pushToMsgQueue(heartbeatFrame, sizeof(heartbeatFrame));
  
  // //3. Switch 1 On 
  //     //7B 00 04 02 00 00 06 7D
  // const byte S1_ON[] = {0x7B, 0x00, 0x04, 0x02, 0x00, 0x00, 0x06 ,0x7D };
  // pushToMsgQueue(S1_ON, sizeof(S1_ON));

  // const byte S1_OFF[] = {0x7B, 0x00, 0x04, 0x02, 0xFF, 0x00, 0x05 ,0x7D };
  // pushToMsgQueue(S1_OFF, sizeof(S1_OFF));

  // const byte S2_ON[] = {0x7B, 0x00, 0x04, 0x03, 0x00, 0x00, 0x07 ,0x7D };
  // pushToMsgQueue(S2_ON, sizeof(S2_ON));

  // const byte S2_OFF[] = {0x7B, 0x00, 0x04, 0x03, 0xFF, 0x00, 0x06 ,0x7D };
  // pushToMsgQueue(S2_OFF, sizeof(S2_OFF));

  // const byte F_ON[] = {0x7B, 0x00, 0x04, 0x01, 0x00, 0x00, 0x05 ,0x7D };
  // pushToMsgQueue(F_ON, sizeof(F_ON));

  // const byte F_ON_25[] = {0x7B, 0x00, 0x04, 0x01, 0x00, 0x19, 0x1E ,0x7D };
  // pushToMsgQueue(F_ON_25, sizeof(F_ON_25));

  // const byte F_ON_50[] = {0x7B, 0x00, 0x04, 0x01, 0x00, 0x32, 0x37 ,0x7D };
  // pushToMsgQueue(F_ON_50, sizeof(F_ON_50));

  // const byte F_OFF[] = {0x7B, 0x00, 0x04, 0x01, 0xFF, 0x00, 0x04 ,0x7D };
  // pushToMsgQueue(F_OFF, sizeof(F_OFF));

  // const byte ALL_ON[] = {0x7B, 0x01, 0x0B, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x6F, 0x7D};
  // pushToMsgQueue(ALL_ON, sizeof(ALL_ON));

  // const byte DEVICE_INFO[] = {0x7B, 0x06, 0x01, 0x01, 0x7D};
  // pushToMsgQueue(DEVICE_INFO, sizeof(DEVICE_INFO));

}

// Function to send Heartbeat (Command 0x00)
void maintainHeartbeat() {

  if (millis() - lastHeartbeatTime >= 15000) {
    FrameHeartbeat();
    lastHeartbeatTime = millis();
    
    #ifdef DEBUG
      Serial.println(F("[TUYA] Heartbeat Sent"));
    #endif
  }
}

// Function to report WiFi Status to MCU (Command 0x03)
void reportWifiStatus() {
  
  if (currentHeartBeatStatus != lastWifiStateReported) {
    lastWifiStateReported = currentHeartBeatStatus;
    
    byte f[8] = {0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, currentHeartBeatStatus, 0x00};
    
    // Calculate Checksum
    byte cs = 0;
    for(int i=0; i<7; i++) cs += f[i];
    f[7] = cs;

    sendFrame(f, sizeof(f),"HWifi Status");

    #ifdef DEBUG
      Serial.print(F("[TUYA] WiFi Status Updated: "));
      Serial.println(currentStatus);
    #endif
  }
}

bool isValidPop(String *Oemdata)
{
  const byte* oem = (const byte*)Oemdata->c_str();
  if(oem[1] == 0x00)
    return true;
  else 
    return false;
}

// The updated Loop
void loop() {

  runStateMachine();      // Your existing State Machine logic
  
  maintainHeartbeat();    // Handles 5s timer for Heartbeat
  
  reportWifiStatus();     // Reports WiFi/MQTT status changes
  //(millis() - lastSerialRead > 500) && 
  if (!msgQueue.empty() && (millis() - lastSerialSend > 100)){
        //Serial.print("Pop the queue ");
        
        // if(ValidPop){
        //   msgQueue.pop_back();
        //   ValidPop = false;
        // }
        // else
        // {
            String nextMsg = msgQueue.front();
            // ValidPop = isValidPop(&nextMsg);
            OemToTuya(&nextMsg);
            msgQueue.pop_front();
            Serial.print(nextMsg);
            lastSerialSend = millis();
        // }
        
      }
  
}