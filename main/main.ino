#include "constant.h"
#include <EEPROM.h>

// 1. Define a safe size (ESP8266 uses one 4KB sector, so 512 is safe)
#define EEPROM_SIZE 512 
// #define DEBUG

void setup() 
{
  Serial.begin(DEBUG_BAUD);
  EEPROM.begin(EEPROM_SIZE);
  #ifdef DEBUG
    Serial.println("--- SYSTEM BOOTING ---");
  #endif

  bool isStateValid = loadStatusFromEEPROM();



   if (isStateValid == false) 
  {
      const byte f[] = {0x55, 0xAA, 0x00, 0x08, 0x00, 0x00 ,0x07};
      size_t length = sizeof(f);

      String msgStr = "";
      msgStr.reserve(length); 

      for (size_t i = 0; i < length; i++) {
        msgStr += (char)f[i];
      }

      msgQueue.push(msgStr);
      
      #ifdef DEBUG
        Serial.println(F("[SYSTEM] First boot/Empty EEPROM: Status Query Queued."));
      #endif
  } 
  else 
  {
      #ifdef DEBUG
        Serial.println(F("[SYSTEM] Restored from EEPROM: Skipping Status Query."));
      #endif
  }

}

void loop() 
{
  runStateMachine(); // Single State Machine for everything.
}