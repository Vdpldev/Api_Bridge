#include "constant.h"

// #define DEBUG

void setup() 
{
  Serial.begin(DEBUG_BAUD);
  #ifdef DEBUG
    Serial.println("--- SYSTEM BOOTING ---");
  #endif
}

void loop() 
{
  runStateMachine(); // Single State Machine for everything.
}