 nbyte serialBuffer[64];
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

bool isResetCommand(byte* buf, int len) 
{
  const byte resetCmd[] = {0x7B, 0x54, 0x02, 0x02, 0x04, 0x7D};
  if (len != 6) return false;
  for (int i = 0; i < 6; i++) 
  {
    if (buf[i] != resetCmd[i]) return false;
  }
  return true;
}

void handleReceivedHexData() 
{
  #ifdef DEBUG  
    Serial.println("\n[SYSTEM] Reset Command Match! Cleaning up...");
  #endif  
  blink100ms(); 
  clearEEPROM();
  ESP.restart();
}

byte lastFrame[64];
int lastLen = 0;
unsigned long lastHeartbeat = 0;

void processSerialInput() 
{
  while (Serial.available() > 0) 
  {
    byte b = Serial.read();
    if (bufferIndex < 64) serialBuffer[bufferIndex++] = b;    //SerialBuffer need to be translate
              
    if (b == '}') 
    {
      if (isResetCommand(serialBuffer, bufferIndex)) 
      {
        handleReceivedHexData();
        bufferIndex = 0;
        return;
      }

      bool hasChanged = false;
      if (bufferIndex != lastLen || memcmp(serialBuffer, lastFrame, bufferIndex) != 0) 
      {
        hasChanged = true;
      }

      bool forceSend = (millis() - lastHeartbeat > 30000);

      if (hasChanged || forceSend) 
      {
        if (mqttClient.connected()) 
        {
          mqttClient.publish(mqtt_pub_topic, serialBuffer, bufferIndex);
          
          memcpy(lastFrame, serialBuffer, bufferIndex);
          lastLen = bufferIndex;
          lastHeartbeat = millis();

          #ifdef DEBUG
            Serial.println("\n[BRIDGE] New/Heartbeat Data sent to MQTT.");
          #endif
        }
      }
      bufferIndex = 0;
    }
  }
}

int captureSerialResponse(uint8_t* buf, size_t maxLen) 
{
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