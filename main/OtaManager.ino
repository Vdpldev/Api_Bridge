#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>


void saveState(){
  EEPROM.put(64, otaState);
  EEPROM.commit();
}

void checkMidnightOTA(){

    time_t now = time(nullptr);
    struct tm *t = localtime(&now);

    if (t->tm_hour == 0 && t->tm_min < 5)
    {
        if (!otaCheckedToday)
        {
            Serial.println("Daily OTA Check");

            currentState = ST_OTA_CHECK;

            otaCheckedToday = true;
        }
    }
    else
    {
        otaCheckedToday = false;
    }
}

void loadState(){
  EEPROM.begin(EEPROM_SIZE);

  EEPROM.get(64, otaState);

  if (otaState.magic != OTA_MAGIC)
  {
    memset(&otaState, 0, sizeof(otaState));

    otaState.magic = OTA_MAGIC;

    saveState();
  }
}

String downloadText(String url){

    Serial.printf(
        "Heap Before: %u\n",
        ESP.getFreeHeap());

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    http.useHTTP10(true);

    http.setFollowRedirects(
        HTTPC_FORCE_FOLLOW_REDIRECTS);

    if(!http.begin(client, url))
    {
        return "";
    }

    int code = http.GET();

    Serial.printf("HTTP Code: %d\n", code);

    String payload = "";

    if(code > 0)
    {
        payload = http.getString();
    }
    else
    {
        Serial.println(
            http.errorToString(code));
    }

    http.end();

    Serial.printf(
        "Heap After: %u\n",
        ESP.getFreeHeap());

    return payload;
}

String getValue(String txt, String key){

  int start = txt.indexOf(key + "=");

  if (start < 0)
    return "";

  start += key.length() + 1;

  int end = txt.indexOf('\n', start);

  if (end < 0)
    end = txt.length();

  return txt.substring(start, end);
}

void updateProgress(int cur, int total){

  int pct = (cur * 100) / total;

  Serial.printf("Progress: %d%%\n", pct);
}

bool checkForUpdates(){
  
  Serial.println();
  Serial.println("Checking Server Version...");

  txt = downloadText(VERSION_URL);
  
  if (txt.length() == 0)
  {
    Serial.println("Version Download Failed");
    return false ;
  }

  String latestVersion = getValue(txt, "version");
  latestVersion.trim();

  String md5 = getValue(txt, "md5");
  md5.trim();

  Serial.print("Version from file = [");
  Serial.print(latestVersion);
  Serial.println("]");

  Serial.print("Current Version = [");
  Serial.print(CURRENT_VERSION);
  Serial.println("]");

  if (latestVersion == CURRENT_VERSION)
  {
    Serial.println("Firmware Up To Date");
    return false;
  }

  Serial.println("Update Available");

  // otaState.updatePending = true;
  // otaState.firmwareConfirmed = false;
  // otaState.bootAttempts = 0;

  // saveState();

  return true;

}

bool performUpdate(String md5) {

  WiFiClientSecure client;
  
  digitalWrite(LED_BUILTIN, LOW); 
  client.setInsecure();

  #ifdef DEBUG  
    Serial.println("[OTA] Starting Download...");
  #endif

  Serial.printf(
    "[OTA] Heap Before OTA: %u\n",
    ESP.getFreeHeap());

  ESPhttpUpdate.onProgress(updateProgress);
  ESPhttpUpdate.setMD5sum(md5.c_str());
  
  ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, FW_URL , CURRENT_VERSION);

  delay(500);
  yield();

  Serial.printf(
    "[OTA] Heap After OTA: %u\n",
    ESP.getFreeHeap());
  Serial.printf(
    "[OTA] Error Code: %d\n",
    ESPhttpUpdate.getLastError());

  Serial.printf(
    "[OTA] Error String: %s\n",
    ESPhttpUpdate.getLastErrorString().c_str());

  switch (ret) 
  {
    case HTTP_UPDATE_FAILED:
      //#ifdef DEBUG
        Serial.printf("[OTA] Update failed. Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      //#endif
      return false;
    break;

    case HTTP_UPDATE_NO_UPDATES:
      //#ifdef DEBUG    
        Serial.println("[OTA] No updates available.");
      //#endif 
      return false;     
    break;
    
    case HTTP_UPDATE_OK:
      //#ifdef DEBUG    
        Serial.println("[OTA] Update successful! Rebooting...");
      //#endif
      return true;
    break;
  }
  return false;
}

void rollbackFirmware(){
  Serial.println("Rolling Back...");

  otaState.updatePending = false;
  otaState.firmwareConfirmed = true;
  otaState.bootAttempts = 0;

  saveState();

  ESP.restart();
}

void confirmFirmware(){

  Serial.println("Update Confirmed");

  otaState.updatePending = false;
  otaState.firmwareConfirmed = true;
  otaState.bootAttempts = 0;

  saveState();
}

void checkHealth(){

  if (!otaState.updatePending)
    return;

  if (millis() - healthCheckStart < 30000)
    return;

  Serial.println("Running Health Check...");

  bool wifiOK = (WiFi.status() == WL_CONNECTED);

  bool appOK = true;

  if (wifiOK)
    Serial.println("WiFi OK");

  if (appOK)
    Serial.println("Application OK");

  if (wifiOK && appOK)
  {
    confirmFirmware();
  }
  else
  {
    Serial.println("Health Check Failed");

    rollbackFirmware();
  }
}



