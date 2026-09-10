unsigned long stateTimer = 0;
static bool apStarted = false;

void runStateMachine() 
{
  switch (currentState) 
  {
    case ST_INIT:
      currentHeartBeatStatus = 0x00;
      #ifdef DEBUG
        Serial.println("[STATE] Initializing Hardware...");
      #endif
      pinMode(STATUS_LED, OUTPUT);
      digitalWrite(STATUS_LED, LOW);
      initStorage();
      initMqtt();
      generateMqttTopics(); 
      //blink100ms();
      currentState = ST_LOAD_CONFIG;
    break;

    case ST_LOAD_CONFIG:
      currentHeartBeatStatus = 0x00;
      if (loadCredentials()) currentState = ST_WIFI_CONNECT;
      else currentState = ST_AP_MODE;
    break;

    case ST_AP_MODE:
      currentHeartBeatStatus = 0x01;
      if (!apStarted) 
      {
        startAPMode();
        apStarted = true;
      }
      handleTcpConfig();
      processSerialInput();
    break;

    case ST_WIFI_CONNECT:
      currentHeartBeatStatus = 0x02;
      digitalWrite(STATUS_LED, LOW);
      //blink400ms();
      startWifiStation(deviceSettings.ssid, deviceSettings.password);
      stateTimer = millis();
      currentState = ST_WIFI_WAITING;
    break;

    case ST_WIFI_WAITING:
      processSerialInput();
      if (getWifiStatus() == WL_CONNECTED) 
      {       
        digitalWrite(STATUS_LED, HIGH);
        currentState = ST_MQTT_CONNECT;
        currentHeartBeatStatus = 0x03;
        
      } 
      else if (millis() - stateTimer >= WIFI_TIMEOUT_MS)
      {
        currentState = ST_WIFI_CONNECT;
      }
    break;

    case ST_MQTT_CONNECT:
      if (attemptMqttConnect())
      {
        currentHeartBeatStatus = 0x04;
        currentState = ST_OPERATIONAL;
      } 
      else 
      {
        stateTimer = millis();
        currentState = ST_IDLE;
      }
    break;
  
    case ST_OPERATIONAL:
      static bool ledReset = false;
      digitalWrite(STATUS_LED, HIGH);
      if (!All_Status) { 
          const byte queryStatusFrame[] =  {0x55, 0xAA, 0x00, 0x08, 0x00, 0x00, 0x07}; 
          sendFrame(queryStatusFrame,sizeof(queryStatusFrame),"All Node Status"); 
          All_Status=true;
        }
      processMqtt();
    
      processSerialInput();

      if (/* specific MQTT command received */ false){
        currentState = ST_OTA_CHECK;
      }

      if (!mqttClient.connected()) 
      {
        ledReset = false;
        currentHeartBeatStatus = 0x03;
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
      if (millis() - stateTimer >= MQTT_RETRY_MS) currentState = ST_WIFI_CONNECT;
      processSerialInput();
    break;    

    case ST_ERROR:
    break;
  }
}