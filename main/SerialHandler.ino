#include <EEPROM.h>

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

void blink400ms()        { const byte f[]={0x7B, 0x05, 0x02, 0x0A, 0x0C, 0x7D};       sendFrame(f, 6, "Blink 400ms");  }
void blink100ms()        { const byte f[]={0x7B, 0x05, 0x03, 0x01, 0x01, 0x05, 0x7D}; sendFrame(f, 7, "Blink 100ms");  }
void blinkoff()          { const byte f[]={0x7B, 0x05, 0x02, 0x00, 0x02, 0x7D};       sendFrame(f, 6, "Blink Off");    }
void requestDeviceInfo() { const byte f[]={0x7B, 0x06, 0x01, 0x01, 0x7D};             sendFrame(f, 5, "Request Info"); }

bool isResetCommand(byte* buf, int len) {
  const byte resetCmd[] = {0x55, 0xAA, 0x03, 0x04, 0x00, 0x00, 0x06};
  if (len != 7) return false;
  for (int i = 0; i < 7; i++) 
  {
    if (buf[i] != resetCmd[i]) return false;
  }
  return true;
}

void saveStatusToEEPROM() {
    currentStatus.signature = STORAGE_SIGNATURE; 
    EEPROM.put(0, currentStatus); // Save struct starting at address 0
    printCurrentStatus("TUYA_MCU"); 
    if (EEPROM.commit()) {                    // Vital for ESP8266/ESP32
        // #ifdef DEBUG
          Serial.println(F("[EEPROM] Data successfully committed to Flash."));
        // #endif
    } else {
        // #ifdef DEBUG
          Serial.println(F("[EEPROM] ERROR: Commit failed!"));
        // #endif
    }              
}
bool loadStatusFromEEPROM() {
    TuyaDeviceState temp;
    EEPROM.get(0, temp); 
    #ifdef DEBUG
      Serial.print(F("[EEPROM] Read Signature: 0x"));
      Serial.println(temp.signature, HEX);
      Serial.print(F("[EEPROM] Expected Signature: 0x"));
      Serial.println(STORAGE_SIGNATURE, HEX);
    #endif
    if (temp.signature == STORAGE_SIGNATURE) {
        currentStatus = temp;
        #ifdef DEBUG
          Serial.println(F("[RECOVERY] Valid State Found. Skipping Query."));
        #endif
        return true; // Success! Data is valid.
    } else {
        #ifdef DEBUG
          Serial.println(F("[RECOVERY] No valid state. Querying MCU..."));
        #endif
        return false; // Failure. EEPROM was empty or corrupted.
    }
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
    }
    
    currentStatus.last_updated = millis();
    currentStatus.is_online = true;
}

void printCurrentStatus(const char* trigger) {
    Serial.print(F("[STATUS UPDATE via "));
    Serial.print(trigger);
    Serial.print(F("] -> "));
    
    Serial.print(F("SW1:")); Serial.print(currentStatus.switch_1 ? "ON " : "OFF ");
    Serial.print(F("SW2:")); Serial.print(currentStatus.switch_2 ? "ON " : "OFF ");
    Serial.print(F("SW3:")); Serial.print(currentStatus.switch_3 ? "ON " : "OFF ");
    Serial.print(F("SW4:")); Serial.print(currentStatus.switch_4 ? "ON " : "OFF ");
    Serial.print(F("| FAN:")); Serial.print(currentStatus.fan_power ? "ON " : "OFF ");
    Serial.print(F("| SPEED:")); Serial.print(currentStatus.fan_speed);
    
    Serial.println();
}

void handleReceivedHexData() {

  const byte f[]= {0x55, 0xAA, 0x00, 0x04, 0x00, 0x00, 0x06};       sendFrame(f, 7, "Response of Reset"); 
  #ifdef DEBUG  
    Serial.println("\n[SYSTEM] Reset Command Match! Cleaning up...");
  #endif  
  blink100ms(); 
  clearEEPROM();
  saveStatusToEEPROM(); 
  ESP.restart();
}

byte lastFrame[64];
int lastLen = 0;
unsigned long lastHeartbeat = 0;

void processSerialInput() {
  
  while (Serial.available() > 0) {
     // 1. Read byte
    byte b = Serial.read();
    digitalWrite(STATUS_LED, LOW); // LED ON while receiving

     // 2. Prevent Buffer Overflow
    if (bufferIndex < 128) { serialBuffer[bufferIndex++] = b;} 
    else {bufferIndex = 0; }// Reset if garbage fills buffer

     // 3. Check Header (Syncing)
    if (bufferIndex == 1 && serialBuffer[0] != 0x55) {bufferIndex = 0;continue;}
    if (bufferIndex == 2 && serialBuffer[1] != 0xAA) {bufferIndex = 0;continue;}

     // 4. Once we have at least the length bytes (Indices 4 and 5)

    if (bufferIndex >= 6) {
      uint16_t dataLen = ((uint16_t)serialBuffer[4] << 8) | serialBuffer[5];
      uint16_t expectedLen = 6 + dataLen + 1; // Header(6) + Payload + Checksum(1)

       // 5. Once the full frame has arrived
      if (bufferIndex == expectedLen) {
        
        if (isResetCommand(serialBuffer, bufferIndex)) {
          handleReceivedHexData();
        }
        
        byte cmd = serialBuffer[3];
        if (cmd == 0x07 || cmd == 0x06) { // Status Report or Command
          int pIdx = 6; // Data starts at index 6
          while (pIdx < (bufferIndex - 1)) { // Loop through all DPIDs in the packet
              byte dpid = serialBuffer[pIdx];
              byte type = serialBuffer[pIdx + 1];
              uint16_t dlen = (serialBuffer[pIdx + 2] << 8) | serialBuffer[pIdx + 3];
              byte* dData = &serialBuffer[pIdx + 4];

              // Update our struct
              updateDeviceState(dpid, type, dData, dlen);
              printCurrentStatus("TUYA_MCU"); 
              pIdx += (4 + dlen); // Jump to next DPID in same packet
          }
        }
         // Debugging output
         //sendFrame(serialBuffer, bufferIndex, "Tuya Inbound");

        // Handle Logic
        int oemLen = 0;
        byte *oemFrame = TuyaToOem(cmd, &serialBuffer[6], dataLen, &oemLen);
        
        
        if (oemFrame != nullptr && oemLen > 0) {
          sendFrame(oemFrame, oemLen, "Converted OEM");

          // Check if data changed or heartbeat (30s) is needed
          bool hasChanged = (bufferIndex != lastLen || memcmp(serialBuffer, lastFrame, bufferIndex) != 0);
          bool forceSend = (millis() - lastHeartbeat > 30000);

          if (hasChanged || forceSend) {
            if (mqttClient.connected()) {
              mqttClient.publish(mqtt_pub_topic, oemFrame, oemLen);
              
              // Update state trackers
              memcpy(lastFrame, serialBuffer, bufferIndex);
              lastLen = bufferIndex;
              lastHeartbeat = millis();
              
              #ifdef DEBUG
                Serial.println(F("[BRIDGE] Data sent to MQTT."));
              #endif
            }
          }
        }

         // Special command handling
        

         // 6. Reset buffer for next packet
        bufferIndex = 0;
      }
    }
  }
}


int captureSerialResponse(uint8_t* buf, size_t maxLen) {
  memset(buf, 0, maxLen); // Clear the buffer first!
  size_t index = 0;
  unsigned long startWait = millis();
  // Wait up to 2 seconds
  while (millis() - startWait < 2000) 
  {
    while (Serial.available()) 
    {
      uint8_t b = Serial.read();
      if (index < maxLen - 1) buf[index++] = b;
      if (b == '}') 
      {
        buf[index] = '\0';
        return index; 
      }
    }
    yield(); // Prevent WDT reset
  }
  return index;
}