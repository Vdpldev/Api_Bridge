unsigned long stateTimer = 0;
static bool apStarted = false;

void runStateMachine() 
{
  switch (currentState) 
  {
    case ST_INIT:
<<<<<<< HEAD
      currentHeartBeatStatus = 0x00;
=======
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      #ifdef DEBUG
        Serial.println("[STATE] Initializing Hardware...");
      #endif
      pinMode(STATUS_LED, OUTPUT);
      digitalWrite(STATUS_LED, LOW);
      initStorage();
      initMqtt();
<<<<<<< HEAD
      generateMqttTopics();
=======
      generateMqttTopics(); 
      blink100ms();
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      currentState = ST_LOAD_CONFIG;
    break;

    case ST_LOAD_CONFIG:
<<<<<<< HEAD
      currentHeartBeatStatus = 0x00;
=======
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      if (loadCredentials()) currentState = ST_WIFI_CONNECT;
      else currentState = ST_AP_MODE;
    break;

    case ST_AP_MODE:
<<<<<<< HEAD
      currentHeartBeatStatus = 0x01;
=======
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      if (!apStarted) 
      {
        startAPMode();
        apStarted = true;
      }
      handleTcpConfig();
<<<<<<< HEAD
      
      if (!All_Status) { 
        const byte queryStatusFrame[] =  {0x55, 0xAA, 0x00, 0x08, 0x00, 0x00, 0x07}; 
        sendFrame(queryStatusFrame,sizeof(queryStatusFrame),"All Node Status"); 
        All_Status=true;
      }
      
=======
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      processSerialInput();
    break;

    case ST_WIFI_CONNECT:
<<<<<<< HEAD
      currentHeartBeatStatus = 0x02;
      digitalWrite(STATUS_LED, LOW);
=======
      blink400ms();
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      startWifiStation(deviceSettings.ssid, deviceSettings.password);
      stateTimer = millis();
      currentState = ST_WIFI_WAITING;
    break;

    case ST_WIFI_WAITING:
<<<<<<< HEAD
      
=======
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      processSerialInput();
      if (getWifiStatus() == WL_CONNECTED) 
      {       
        digitalWrite(STATUS_LED, HIGH);
        currentState = ST_MQTT_CONNECT;
<<<<<<< HEAD
        currentHeartBeatStatus = 0x03;
        
=======
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      } 
      else if (millis() - stateTimer >= WIFI_TIMEOUT_MS)
      {
        currentState = ST_WIFI_CONNECT;
      }
    break;

    case ST_MQTT_CONNECT:
      if (attemptMqttConnect())
      {
<<<<<<< HEAD
        
        currentHeartBeatStatus = 0x04;
=======
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
        currentState = ST_OPERATIONAL;
      } 
      else 
      {
        stateTimer = millis();
        currentState = ST_IDLE;
      }
    break;
<<<<<<< HEAD
  
    case ST_OPERATIONAL:
      
      if (!All_Status) { 
        const byte queryStatusFrame[] =  {0x55, 0xAA, 0x00, 0x08, 0x00, 0x00, 0x07}; 
        sendFrame(queryStatusFrame,sizeof(queryStatusFrame),"All Node Status"); 
        All_Status=true;
      }
      digitalWrite(STATUS_LED, HIGH);
      
      processMqtt();
    
      processSerialInput();

      if (/* specific MQTT command received */ false){
=======

    case ST_OPERATIONAL:
      static bool ledReset = false;
      if(!ledReset) { blinkoff(); ledReset = true; }
      digitalWrite(STATUS_LED, HIGH);
      processMqtt();
      processSerialInput();

      static unsigned long lastSerialSend = 0;
      if (!msgQueue.empty() && (millis() - lastSerialSend > 100))
      {
        String nextMsg = msgQueue.front();
        Serial.print(nextMsg);
        msgQueue.pop();
        lastSerialSend = millis();
      }

      if (/* specific MQTT command received */ false)
      {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
        currentState = ST_OTA_CHECK;
      }

      if (!mqttClient.connected()) 
      {
<<<<<<< HEAD
        currentHeartBeatStatus = 0x03;
=======
        ledReset = false;
        blink400ms();
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
        currentState = ST_MQTT_CONNECT;
      }
    break;

    case ST_OTA_CHECK:
      if (checkForUpdates()) 
      {
        currentState = ST_OTA_PERFORM;
      }
      else
      {
        currentState = ST_OPERATIONAL;
      }
    break;

    case ST_OTA_PERFORM:
      performUpdate();
      currentState = ST_OPERATIONAL;
    break;

    case ST_IDLE: // This is our "Retry Wait" state
<<<<<<< HEAD
      if (millis() - stateTimer >= MQTT_RETRY_MS) currentState = ST_WIFI_CONNECT;
=======
      if (millis() - stateTimer >= MQTT_RETRY_MS) currentState = ST_MQTT_CONNECT;
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      processSerialInput();
    break;    

    case ST_ERROR:
    break;
  }
}