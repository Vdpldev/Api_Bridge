byte serialBuffer[128];

int bufferIndex = 0;

void sendFrame(const byte* frame, size_t len, const char* debugMsg) 
{
  Serial.write(frame, len);
  #ifdef DEBUG  
    Serial.print("\n[MCU] Sent: ");
    Serial.println(debugMsg);
  #endif

}

void blink400ms()        { const byte f[]={0x7B, 0x05, 0x02, 0x0A, 0x0C, 0x7D};                       sendFrame(f, 6, "Blink 400ms");  }
void blink100ms()        { const byte f[]={0x7B, 0x05, 0x03, 0x01, 0x01, 0x05, 0x7D};                 sendFrame(f, 7, "Blink 100ms");  }
void blinkoff()          { const byte f[]={0x7B, 0x05, 0x02, 0x00, 0x02, 0x7D};                       sendFrame(f, 6, "Blink Off");    }
void requestDeviceInfo() { const byte DEVICE_INFO[] = {0x55, 0xAA, 0x00, 0x01, 0x00, 0x00, 0x00};     sendFrame(DEVICE_INFO, sizeof(DEVICE_INFO),"Device Information"); }
void FrameHeartbeat()    { const byte Heartbeat[] = {0x55, 0xAA, 0x00, TUYA_CMD_HEARTBEAT, 0x00, 0x00, 0xFF};     sendFrame(Heartbeat, sizeof(Heartbeat),"Heart Beat Frame"); }

bool isResetCommand(byte* buf, int len) {
  const byte resetCmd[] = {0x55, 0xAA, 0x03, 0x04, 0x00, 0x00, 0x06};
  if (len != 7) return false;
  for (int i = 0; i < 7; i++) 
  {
    if (buf[i] != resetCmd[i]) return false;
  }
  return true;
}

void updateDeviceState(byte dpid, byte type, byte* data, int len) {
    
    // Type 0x01 = Boolean, 0x02 = Value (4 bytes), 0x04 = Enum (1 byte)
    
    switch (dpid) {
        case 0x01: // Switch 1
            currentStatus.switch_1 = (data[0] == 0x01);
            break;
        case 0x02: // Switch 2
            currentStatus.switch_2 = (data[0] == 0x01);
            break;
        case 0x03: // Switch 2
            currentStatus.switch_3 = (data[0] == 0x01);
            break;
        case 0x04: // Switch 2
            currentStatus.switch_4 = (data[0] == 0x01);
            break;
        case 0x66: // Fan Power (DPID 102)
            currentStatus.fan_power = (data[0] == 0x01);
            break;
        case 0x68: // Fan Speed (DPID 104)
            // Tuya 'Value' types are 4 bytes long (Big Endian)
            currentStatus.fan_speed = (uint8_t)data[len-1]; 
            break;
        
        case 0x10: // Switch backlight
            currentStatus.switch_BL = (data[0] == 0x01); 
            break;
          
        case 0x65: // Child Lock
            currentStatus.child_lock = (data[0] == 0x01) ; 
            break;
          
        case 0x0E: // Fan Speed (DPID 104)
            // Tuya 'Value' types are 4 bytes long (Big Endian)
            currentStatus.restart_Status =  (uint8_t)data[len-1]; 
            break;
        
        ;

    }
    
}

void printCurrentStatus(const char* trigger) {
    Serial.print(F("[STATUS UPDATE via "));
    Serial.print(trigger);
    Serial.print(F("] -> "));
    
    Serial.print(F("SW1:")); Serial.print(currentStatus.switch_1 ? "ON " : "OFF ");
    Serial.print(F("SW2:")); Serial.print(currentStatus.switch_2 ? "ON " : "OFF ");
    Serial.print(F("SW3:")); Serial.print(currentStatus.switch_3 ? "ON " : "OFF ");
    Serial.print(F("SW4:")); Serial.print(currentStatus.switch_4 ? "ON " : "OFF ");
    Serial.print(F("| BackLight:")); Serial.print(currentStatus.switch_BL ? "ON " : "OFF ");
    Serial.print(F("| Child_lock:")); Serial.print(currentStatus.child_lock ? "ON " : "OFF ");
    Serial.print(F("| FAN:")); Serial.print(currentStatus.fan_power ? "ON " : "OFF ");
    Serial.print(F("| SPEED:")); Serial.print(currentStatus.fan_speed);
    Serial.print(F("| Restart Status:")); Serial.print(currentStatus.restart_Status);
    
    Serial.println();
}

void handleReceivedHexData() {

  #ifdef DEBUG  
    Serial.println("\n[SYSTEM] Reset Command Match! Cleaning up...");
  #endif  
  blink100ms(); 
  clearEEPROM();
  ESP.restart();
}



void processSerialInput() {
  
  while (Serial.available() > 0) {
     // 1. Read byte
    byte b = Serial.read();

     // 2. Prevent Buffer Overflow
    if (bufferIndex < 128) { serialBuffer[bufferIndex++] = b;} 
    else {bufferIndex = 0; }// Reset if garbage fills buffer

     // 3. Check Header (Syncing)
    if (bufferIndex == 1 && serialBuffer[0] != TUYA_HEADER_HIGH) {bufferIndex = 0;continue;}
    if (bufferIndex == 2 && serialBuffer[1] != TUYA_HEADER_LOW) {bufferIndex = 0;continue;}

     // 4. Once we have at least the length bytes (Indices 4 and 5)

    if (bufferIndex >= 6) {
      uint16_t dataLen = ((uint16_t)serialBuffer[4] << 8) | serialBuffer[5];
      uint16_t expectedLen = 6 + dataLen + 1; // Header(6) + Payload + Checksum(1)

       // 5. Once the full frame has arrived
      if (bufferIndex == expectedLen) {
        
        byte calculatedTuyaCs = 0;
        for (int i = 0; i < expectedLen - 1; i++) {
            calculatedTuyaCs += serialBuffer[i];
        }

        if (calculatedTuyaCs != serialBuffer[expectedLen - 1]) {
          #ifdef DEBUG
              Serial.println(F("[ERROR] Tuya Checksum Mismatch! Discarding."));
          #endif
          break;
        }

        if (isResetCommand(serialBuffer, bufferIndex)) {
          handleReceivedHexData();
          return;
        }
        byte ver = serialBuffer[2];
        byte cmd = serialBuffer[3];
        
        // Handle Logic
        int oemLen = 0;
        byte *oemFrame = TuyaToOem(ver ,cmd, &serialBuffer[6], dataLen, &oemLen);
        
        
        if (oemFrame != nullptr && oemLen > 0) {
          
          #ifdef DEBUG
            sendFrame(oemFrame, oemLen, "Converted OEM");
          #endif
          
          // Check if data changed or heartbeat (30s) is needed
          bool hasChanged = (bufferIndex != lastLen || memcmp(serialBuffer, lastFrame, bufferIndex) != 0);
          bool forceSend = (millis() - lastHeartbeat > 30000);

          if (hasChanged || forceSend) {
            if (mqttClient.connected()) {
              mqttClient.publish(mqtt_pub_topic, oemFrame, oemLen);
              
              // Update state trackers
              memcpy(lastFrame, oemFrame, oemLen);
              lastLen = bufferIndex;
              lastHeartbeat = millis();
              
              #ifdef DEBUG
                Serial.println(F("[BRIDGE] Data sent to MQTT."));
              #endif
            }
          }
        }

         // Special command handling
        lastSerialRead = millis();

         // 6. Reset buffer for next packet
        bufferIndex = 0;
      }
    }
  }
}


int captureSerialResponse(uint8_t* buf, size_t maxLen) {
  memset(buf, 0, maxLen); // Clear the buffer first!
  size_t bufferIndex = 0;
  int oemLen = 0;
  unsigned long startWait = millis();
  // Wait up to 2 seconds
  while (millis() - startWait < 5000) 
  {
    while (Serial.available()) 
    {
        uint8_t b = Serial.read();
        if (bufferIndex < maxLen - 1) serialBuffer[bufferIndex++] = b;
      
          // 3. Check Header (Syncing)
        if (bufferIndex == 1 && serialBuffer[0] != TUYA_HEADER_HIGH) {bufferIndex = 0;continue;}
        if (bufferIndex == 2 && serialBuffer[1] != TUYA_HEADER_LOW) {bufferIndex = 0;continue;}

          // 4. Once we have at least the length bytes (Indices 4 and 5)

        if (bufferIndex >= 6) {
          uint16_t dataLen = ((uint16_t)serialBuffer[4] << 8) | serialBuffer[5];
          uint16_t expectedLen = 6 + dataLen + 1; // Header(6) + Payload + Checksum(1)

            // 5. Once the full frame has arrived
          if (bufferIndex == expectedLen) {
            
            byte calculatedTuyaCs = 0;
            for (int i = 0; i < expectedLen - 1; i++) {
                calculatedTuyaCs += serialBuffer[i];
            }

            if (calculatedTuyaCs != serialBuffer[expectedLen - 1]) {
              #ifdef DEBUG
                  Serial.println(F("[ERROR] Tuya Checksum Mismatch! Discarding."));
              #endif
              break;
            }

            if (isResetCommand(serialBuffer, bufferIndex)) {
              handleReceivedHexData();
              return 0;
            }
            byte ver = serialBuffer[2];
            byte cmd = serialBuffer[3];
            
            // Handle Logic
            uint8_t* oemFrame = TuyaToOem(ver, cmd, &serialBuffer[6], dataLen, &oemLen);
            memcpy(buf, oemFrame, oemLen);
            return oemLen;
            
              // Special command handling
            lastSerialRead = millis();

              // 6. Reset buffer for next packet
            bufferIndex = 0;
          }
          yield(); // Prevent WDT reset
        }
    }
  }
  return oemLen;
}