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

  bool update_available = doc["firmware_id"].as<bool>();
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

  if (!update_available)
  {
    //#ifdef DEBUG
    Serial.println("Firmware Up To Date");
    //#endif
    return update_available;
  }

  Serial.println("Update Available");

  // otaState.updatePending = true;
  // otaState.firmwareConfirmed = false;
  // otaState.bootAttempts = 0;

  // saveState();

  return update_available;

}

bool performUpdate() {

  WiFiClientSecure httpsClient;
  sendUpdateStatus("downloading", firmware_id, "none");
  delay(100);
  
  String firmwareURL = String(OTA_URL) + "/api/firmware/" + firmware_id + "/download";

  digitalWrite(LED_BUILTIN, LOW); 
  httpsClient.setInsecure();
  HTTPClient https;

  

  if (!https.begin(httpsClient, firmwareURL)) {
    Serial.println("❌ HTTPS begin failed");
    sendUpdateStatus("failed", firmware_id, "HTTPS begin failed");
    delay(100);
    return false;
  }

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("❌ HTTP GET failed, code: %d\n", httpCode);
    sendUpdateStatus("failed", firmware_id, "HTTP GET failed: " + String(httpCode));
    delay(100);
    https.end();
    return false;
  }

  int totalLength = https.getSize();
  if (totalLength <= 0) {
    Serial.println("❌ Invalid firmware size");
    sendUpdateStatus("failed", firmware_id, "Invalid firmware size");
    delay(100);
    https.end();
    return false;
  }

   //   DEBUG.printf("Firmware size: %d bytes\n", totalLength);

  if (!Update.begin(totalLength)) {
    Serial.println("❌ Not enough space for OTA");
    sendUpdateStatus("failed", firmware_id, "Not enough space for OTA");
    delay(100);
    https.end();
    return false;
  }

  // SHA256 init
  br_sha256_context ctx;
  br_sha256_init(&ctx);

  WiFiClient* stream = https.getStreamPtr();
  uint8_t buff[512];
  int written = 0;
  int lastReportedPercent = 0;

  while (https.connected() && written < totalLength) {
    size_t sizeAvailable = stream->available();
    if (sizeAvailable) {
      int bytesRead = stream->readBytes(buff, (sizeAvailable > sizeof(buff)) ? sizeof(buff) : sizeAvailable);
      br_sha256_update(&ctx, buff, bytesRead);
      Update.write(buff, bytesRead);
      written += bytesRead;

      int percent = (written * 100) / totalLength;
      if (percent - lastReportedPercent >= 10 || percent == 100) {
        lastReportedPercent = percent;
        String msg = "Downloaded " + String(written) + " of " + String(totalLength) + " bytes (" + String(percent) + "%)";
        Serial.println(msg);
        // sendUpdateStatus("downloading", firmware_id, msg);
        delay(100);
      }
    }
    delay(1);
  }

  

  // Finalize SHA256
  uint8_t hash[32];
  br_sha256_out(&ctx, hash);

  String calculatedChecksum = "";
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 0x10) calculatedChecksum += "0";
    calculatedChecksum += String(hash[i], HEX);
  }
  calculatedChecksum.toLowerCase();

  Serial.println("Expected checksum:   " + expectedChecksum);
  Serial.println("Calculated checksum: " + calculatedChecksum);

  if (expectedChecksum != calculatedChecksum) {
    Serial.println("❌ Checksum mismatch! Aborting update.");
    sendUpdateStatus("failed", firmware_id, "Checksum mismatch");
    delay(100);
    Update.end(); // do not commit
    return false;
  }

  if (!Update.end()) {
    Serial.printf("❌ OTA update error: %s\n", Update.getErrorString());
    sendUpdateStatus("failed", firmware_id, Update.getErrorString());
    delay(100);
    return false;
  }

  Serial.println("✅ OTA update successful. Rebooting...");
     sendUpdateStatus("success", firmware_id, "Update completed successfully");
  delay(100);
  https.end();
  ESP.restart();
  return true;

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



