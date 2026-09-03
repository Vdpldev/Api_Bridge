// ✅ Continuously check Serial input even in AP Mode
void checkSerialWhileAP() 
{
    while (Serial.available() > 0) 
    {
      byte incomingByte = Serial.read();
      if (bufferIndex < bufferSize) 
      {
        buffer[bufferIndex++] = incomingByte;
      }
      if (isResetCommand(buffer, bufferIndex)) 
      {
        handleReceivedHexData();
        bufferIndex = 0;
      }
    }
}

void blink400ms() 
{
  const byte confirmationFrame[] = {0x7B, 0x05, 0x02, 0x0A, 0x0C, 0x7D};
  for (byte b : confirmationFrame) 
  {
    Serial.write(b);
  }
  DEBUG.println("Confirmation frame sent.");
}

bool isResetCommand(byte* buffer, int length) 
{
  const byte resetCommand[] = {0x7B, 0x54, 0x02, 0x02, 0x04, 0x7D};
  if (length != sizeof(resetCommand)) 
  {
    return false;
  }
  for (int i = 0; i < length; i++) 
  {
    if (buffer[i] != resetCommand[i]) 
    {
      return false;
    }
  }
  return true;
}

void blink100ms() 
{
  const byte confirmationFrame[] = {0x7B, 0x05, 0x03, 0x01,0x01, 0x05, 0x7D};
  for (byte b : confirmationFrame) 
  {
    Serial.write(b);
  }
  DEBUG.println("Confirmation frame sent.");
}


void gatedeviceinfo() 
{
  const byte confirmationFrame[] = {0x7B, 0x06, 0x01, 0x01, 0x7D};
  for (byte b : confirmationFrame)
  {
    Serial.write(b);
  }
  DEBUG.println("Confirmation frame sent.");
}

// Handle received hex frame and reset the ESP
void handleReceivedHexData() 
{
    // Clear EEPROM and reset the ESP8266
    clearEEPROM();
    DEBUG.println("EEPROM cleared due to received trigger data.");

    // Send response data to Serial
    DEBUG.println("Response data sent to Serial.");
    blink100ms();
    // Reset the ESP8266
    DEBUG.println("Resetting the ESP8266...");
    ESP.restart();
}

void blinkoff() 
{
  const byte confirmationFrame[] = {0x7B, 0x05, 0x02, 0x00, 0x02, 0x7D};
  for (byte b : confirmationFrame) 
  {
    Serial.write(b);
  }
  DEBUG.println("Confirmation frame sent.");
}