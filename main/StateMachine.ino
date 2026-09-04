unsigned long stateTimer = 0;
static bool apStarted = false;

void runStateMachine() 
{
  switch (currentState) 
  {
    case ST_INIT:
      #ifdef DEBUG
        Serial.println("[STATE] Initializing Hardware...");
      #endif
      pinMode(STATUS_LED, OUTPUT);
      digitalWrite(STATUS_LED, LOW);
      initStorage();
      initMqtt();
      generateMqttTopics(); 
      blink100ms();
      currentState = ST_LOAD_CONFIG;
    break;

    case ST_LOAD_CONFIG:
      if (loadCredentials()) currentState = ST_WIFI_CONNECT;
      else currentState = ST_AP_MODE;
    break;

    case ST_AP_MODE:
      if (!apStarted) 
      {
        startAPMode();
        apStarted = true;
      }
      handleTcpConfig();
      processSerialInput();
    break;

    case ST_WIFI_CONNECT:
      blink400ms();
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
      } 
      else if (millis() - stateTimer >= WIFI_TIMEOUT_MS)
      {
        currentState = ST_WIFI_CONNECT;
      }
    break;

    case ST_MQTT_CONNECT:
      if (attemptMqttConnect())
      {
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
      //if(!ledReset) { blinkoff(); ledReset = true; }
      //digitalWrite(STATUS_LED, HIGH);
      //processMqtt();
    
      processSerialInput();
      //digitalWrite(STATUS_LED, LOW);
      static unsigned long lastSerialSend = 0;

      if (!msgQueue.empty() && (millis() - lastSerialSend > 100))
      {
        //Serial.print("Pop the queue ");
        String nextMsg = msgQueue.front();
        OemToTuya(&nextMsg);
        Serial.print(nextMsg);
        msgQueue.pop();
        lastSerialSend = millis();
      }

      if (/* specific MQTT command received */ false){
        currentState = ST_OTA_CHECK;
      }

      // if (!mqttClient.connected()) 
      // {
      //   ledReset = false;
      //   blink400ms();
      //   currentState = ST_MQTT_CONNECT;
      // }
    break;

    // case ST_OTA_CHECK:
    //   if (checkForUpdates()) 
    //   {
    //     currentState = ST_OTA_PERFORM;
    //   }
    //   else
    //   {
    //     currentState = ST_OPERATIONAL;
    //   }
    // break;

    // case ST_OTA_PERFORM:
    //   performUpdate();
    //   currentState = ST_OPERATIONAL;
    // break;

    // case ST_IDLE: // This is our "Retry Wait" state
    //   if (millis() - stateTimer >= MQTT_RETRY_MS) currentState = ST_MQTT_CONNECT;
    //   processSerialInput();
    // break;    

    // case ST_ERROR:
    // break;
  }
}