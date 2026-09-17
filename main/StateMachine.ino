unsigned long stateTimer = 0;
static bool apStarted = false;

void runStateMachine() 
{
  switch (currentState) 
  {
    case ST_INIT:
      currentHeartbeatStatus = 0x00;
      #ifdef DEBUG
        Serial.println("[STATE] Initializing Hardware...");
      #endif
      pinMode(STATUS_LED, OUTPUT);
      digitalWrite(STATUS_LED, LOW);
      initStorage();
      initMqtt();
      generateMqttTopics();
      currentState = ST_LOAD_CONFIG;
    break;

    case ST_LOAD_CONFIG:
      currentHeartbeatStatus = 0x00;
      if (loadCredentials()) currentState = ST_WIFI_CONNECT;
      else currentState = ST_AP_MODE;
    break;

    case ST_AP_MODE:
      currentHeartbeatStatus = 0x01;
      if (!apStarted) 
      {
        startAPMode();
        apStarted = true;
      }
      handleTcpConfig();
      
      if (!allNodesStatusReceived) { 
        sendFrame(queryFrame,sizeof(queryFrame),"All Node Status"); 
        allNodesStatusReceived=true;
      }
      
      processSerialInput();
    break;

    case ST_WIFI_CONNECT:
      currentHeartbeatStatus = 0x02;
      digitalWrite(STATUS_LED, LOW);
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
        currentHeartbeatStatus = 0x03;
        configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST (+5:30)
        
      } 
      else if (millis() - stateTimer >= WIFI_TIMEOUT_MS)
      {
        currentState = ST_WIFI_CONNECT;
      }
    break;

    case ST_MQTT_CONNECT:
      if (attemptMqttConnect())
      {
        
        currentHeartbeatStatus = 0x04;
        currentState = ST_OPERATIONAL;
      } 
      else 
      {
        stateTimer = millis();
        currentState = ST_IDLE;
      }
    break;
  
    case ST_OPERATIONAL:
      
      if (!allNodesStatusReceived) { 
        sendFrame(queryFrame,sizeof(queryFrame),"All Node Status"); 
        allNodesStatusReceived=true;
      }
      digitalWrite(STATUS_LED, HIGH);
      
      processMqtt();
    
      processSerialInput();

      checkMidnightOTA();
      
      if (false){
        Serial.print("checking the OTA");
        currentState = ST_OTA_CHECK;
      }

      if (!mqttClient.connected()) 
      {
        currentHeartbeatStatus = 0x03;
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
        stateTimer = millis();
      }
    break;

    case ST_OTA_PERFORM:{

      bool ok = performUpdate(getValue(txt, "md5"));
      
      if (!ok)
      {
        otaState.updatePending = false;

        saveState();

        Serial.println("OTA Aborted");
      }
      currentState = ST_INIT;
      stateTimer = millis();

    break;}

    case ST_IDLE: // This is our "Retry Wait" state
      if (millis() - stateTimer >= MQTT_RETRY_MS) currentState = ST_WIFI_CONNECT;
      processSerialInput();
    break;    

    case ST_ERROR:
    break;
  }
}