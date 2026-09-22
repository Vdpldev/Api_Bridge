#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>

String firmware_id; 
String expectedChecksum ; // store checksum from serve


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
          #ifdef DEBUG
            Serial.println("Daily OTA Check");
          #endif

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

    #ifdef DEBUG
        Serial.printf(
            "Heap Before: %u\n",
            ESP.getFreeHeap());
    #endif
    
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    http.useHTTP10(true);

    http.setFollowRedirects(
        HTTPC_FORCE_FOLLOW_REDIRECTS);

    if(!http.begin(client, url)) return "";
    

    int code = http.GET();

    #ifdef DEBUG
        Serial.printf("HTTP Code: %d\n", code);
    #endif
    String payload = "";

    if(code > 0) payload = http.getString();
    else
    {
      #ifdef DEBUG
        Serial.println(
            http.errorToString(code));
      #endif
    }

    http.end();

    #ifdef DEBUG
      Serial.printf(
          "Heap After: %u\n",
          ESP.getFreeHeap());
    #endif

    return payload;
}

String getValue(String txt, String key){

  int start = txt.indexOf(key + "=");

  if (start < 0)  return "";

  start += key.length() + 1;

  int end = txt.indexOf('\n', start);

  if (end < 0)  end = txt.length();

  return txt.substring(start, end);
}

void updateProgress(int cur, int total){

  int pct = (cur * 100) / total;
  if(pct == 50 ) sendUpdateStatus("installing", firmware_id, "none");
  
  Serial.printf("Progress: %d%%\n", pct);
}

void sendUpdateStatus(String status, String firmware_id, String message) {
    
    #ifdef DEBUG
    Serial.println("  Sending update status:");
    Serial.println("  Status: " + status);
    Serial.println("  Firmware ID: " + firmware_id);
    Serial.println("  Message: " + message);
    #endif
    String mac = WiFi.macAddress();
    mac.toLowerCase();  // Ensure MAC is in lowercase

    // Create JSON payload
    StaticJsonDocument<1024> doc;  // Increased buffer size for safety
    doc["mac_address"] = mac;
    doc["firmware_id"] = firmware_id;
    doc["status"] = status;
    doc["error_message"] = message;

    String payload;
    serializeJson(doc, payload);

    #ifdef DEBUG
      Serial.println("  Payload: " + payload);
    #endif

    String statusURL = String(OTA_URL) + "/api/ota/update-status";

    HTTPClient https;
    WiFiClientSecure httpsClient;
    httpsClient.setInsecure();

    if (!https.begin(httpsClient, statusURL)) {
        #ifdef DEBUG
          Serial.println(" Failed to connect to status URL");
        #endif
        return;
    }

    https.addHeader("Content-Type", "application/json");  //  Required for JSON POST

    int httpCode = https.POST(payload);

    if (httpCode > 0) {
        #ifdef DEBUG
        Serial.printf(" Status update sent (%d): %s\n", httpCode, https.getString().c_str());
        #endif
    } else {
      #ifdef DEBUG
        Serial.printf(" Failed to send status update. Error: %s\n", https.errorToString(httpCode).c_str());
      #endif
    }

    // https.end();
}

bool checkForUpdates(){
  
  WiFiClientSecure httpsClient;

  String mac = WiFi.macAddress();
  String url = String(OTA_URL) + "/api/ota/checkupdate?mac_address=" + mac +
                 "&current_version=" + String(CURRENT_VERSION);

  HTTPClient https;
    httpsClient.setInsecure();
    if (!https.begin(httpsClient, url)) {
      #ifdef DEBUG
        Serial.println("❌ HTTPS begin failed");
      #endif
        return false ;
    }
  
    delay(100);

    https.addHeader("User-Agent", "ESP8266-http-Update");
    int httpCode = https.GET();
    if (httpCode <= 0) {

      #ifdef DEBUG
        Serial.printf("❌ HTTP GET failed. Error: %s\n", https.errorToString(httpCode).c_str());
      #endif
        https.end();
        return false;

    }
  #ifdef DEBUG
    Serial.printf("HTTP Status: %d\n", httpCode);
  #endif

  String payload = https.getString();

  #ifdef DEBUG
      Serial.println("Response:");
      Serial.println(payload);
  #endif

  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    #ifdef DEBUG
      Serial.println("❌ JSON parse failed");
    #endif
      return false;
  }

  

  firmware_id = doc["firmware_id"].as<String>();
  expectedChecksum  = doc["checksum"].as<String>();
  String latestVersion = doc["version"].as<String>();
  latestVersion.trim();
  md5 = doc["checksum"].as<String>();
  md5.trim();
  // #ifdef DEBUG
  //    Serial.println();
  //    Serial.println("Checking Server Version...");
  // #endif

  // txt = downloadText(VERSION_URL);
  
  // if (txt.length() == 0)
  // {
  // #ifdef DEBUG
  //    Serial.println("Version Download Failed");
  // #endif
  //   return false ;
  // }

  // String latestVersion = getValue(txt, "version");
  // latestVersion.trim();

  // String md5 = getValue(txt, "md5");
  // md5.trim();

  // Serial.print("Version from file = [");
  // #ifdef DEBUG
  //    Serial.print(latestVersion);
  //    Serial.println("]");
  // #endif

  // Serial.print("Current Version = [");
  // #ifdef DEBUG
  //    Serial.print(CURRENT_VERSION);
  //    Serial.println("]");
  // #endif

  if (latestVersion == CURRENT_VERSION)
  {
    #ifdef DEBUG
    Serial.println("Firmware Up To Date");
    #endif
    return false;
  }

  Serial.println("Update Available");

  // otaState.updatePending = true;
  // otaState.firmwareConfirmed = false;
  // otaState.bootAttempts = 0;

  // saveState();

  return true;

}

bool performUpdate() {

  WiFiClientSecure client;
  sendUpdateStatus("downloading", firmware_id, "none");
  delay(100);
  
  String firmwareURL = String(OTA_URL) + "/api/firmware/" + firmware_id + "/download";

  digitalWrite(LED_BUILTIN, LOW); 
  client.setInsecure();

  #ifdef DEBUG  
    Serial.println("[OTA] Starting Download...");
  #endif
  #ifdef DEBUG  
    Serial.printf(
      "[OTA] Heap Before OTA: %u\n",
      ESP.getFreeHeap());
  #endif

  ESPhttpUpdate.onProgress(updateProgress);
  ESPhttpUpdate.setMD5sum(md5.c_str());
  
  ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, firmwareURL , CURRENT_VERSION);

  delay(500);
  yield();

  #ifdef DEBUG  

    Serial.printf(
      "[OTA] Heap After OTA: %u\n",
      ESP.getFreeHeap());
    Serial.printf(
      "[OTA] Error Code: %d\n",
      ESPhttpUpdate.getLastError());

    Serial.printf(
      "[OTA] Error String: %s\n",
      ESPhttpUpdate.getLastErrorString().c_str());

  #endif

  switch (ret) 
  {
    case HTTP_UPDATE_FAILED:
        sendUpdateStatus("failed", firmware_id, "none");
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
        sendUpdateStatus("success", firmware_id, "none");
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



