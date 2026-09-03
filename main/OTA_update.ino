#include <bearssl/bearssl_hash.h>
WiFiClientSecure httpsClient; // keep it alive
String firmware_id; 
String expectedChecksum ; // store checksum from serve


void checkAndUpdate() {
    DEBUG.println("\n--- Starting OTA Update Check ---");

    String mac = WiFi.macAddress();
    mac.toLowerCase();
    String url = String(baseURL) + "/api/ota/checkupdate?mac_address=" + mac +
                 "&current_version=" + current_version;

    DEBUG.println("Update check URL: " + url);
    

    HTTPClient https;
    httpsClient.setInsecure();
    if (!https.begin(httpsClient, url)) {
        DEBUG.println("❌ HTTPS begin failed");
        return;
    }
    delay(100);

    https.addHeader("User-Agent", "ESP8266-http-Update");
    int httpCode = https.GET();
    if (httpCode <= 0) {
        DEBUG.printf("❌ HTTP GET failed. Error: %s\n", https.errorToString(httpCode).c_str());
        https.end();
        return;
    }

    DEBUG.printf("HTTP Status: %d\n", httpCode);
    String payload = https.getString();
    DEBUG.println("Response:");
    DEBUG.println(payload);

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        DEBUG.println("❌ JSON parse failed");
        return;
    }

    

    firmware_id = doc["firmware_id"].as<String>();
    expectedChecksum  = doc["checksum"].as<String>(); // store checksum
    expectedChecksum.toLowerCase();
     DEBUG.println("Firmware ID: " + firmware_id);
    DEBUG.println("Expected checksum: " + expectedChecksum); // <-- Now valid

    // delay(2000);
    delay(2000); // give TLS and server some breathing room
    
}


void rportupdate() {
    DEBUG.println("\n--- Send OTA Update CURRENT FIRMWARE VERSION ---");

    
    String mac = WiFi.macAddress();
    mac.toLowerCase();
    String url = String(baseURL) + "/api/ota/ota-complete?mac_address=" + mac +
                 "&current_version=" + current_version;

    DEBUG.println("Update check URL: " + url);
    

    HTTPClient https;
    httpsClient.setInsecure();
    if (!https.begin(httpsClient, url)) {
        DEBUG.println("❌ HTTPS begin failed");
        return;
    }
    delay(100);

    https.addHeader("User-Agent", "ESP8266-http-Update");
    int httpCode = https.GET();
    if (httpCode <= 0) {
        DEBUG.printf("❌ HTTP GET failed. Error: %s\n", https.errorToString(httpCode).c_str());
        https.end();
        return;
    }

    DEBUG.printf("HTTP Status: %d\n", httpCode);
    String payload = https.getString();
    DEBUG.println("Response:");
    DEBUG.println(payload);

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        DEBUG.println("❌ JSON parse failed");
        return;
    }

    // delay(2000);
    delay(2000); // give TLS and server some breathing room
    
}

void sendUpdateStatus(String status, String firmware_id, String message) {
    DEBUG.println("📤 Sending update status:");
    DEBUG.println("  Status: " + status);
    DEBUG.println("  Firmware ID: " + firmware_id);
    DEBUG.println("  Message: " + message);

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

    DEBUG.println("  Payload: " + payload);

    String statusURL = String(baseURL) + "/api/ota/update-status";

    HTTPClient https;
    httpsClient.setInsecure();

    if (!https.begin(httpsClient, statusURL)) {
        DEBUG.println("❌ Failed to connect to status URL");
        return;
    }

    https.addHeader("Content-Type", "application/json");  // ✅ Required for JSON POST

    int httpCode = https.POST(payload);

    if (httpCode > 0) {
        DEBUG.printf("✅ Status update sent (%d): %s\n", httpCode, https.getString().c_str());
    } else {
        DEBUG.printf("❌ Failed to send status update. Error: %s\n", https.errorToString(httpCode).c_str());
    }

    // https.end();
}

bool verifyFirmwareChecksum(const String& expectedChecksum) {
  
    DEBUG.println("Verifying firmware checksum...");
    
    File f = SPIFFS.open("/firmware.bin", "r");
    if (!f) {
        DEBUG.println("❌ Firmware file not found for checksum verification!");
        return false;
    }

    br_sha256_context ctx;
    uint8_t hash[32];
    br_sha256_init(&ctx);

    while (f.available()) {
        uint8_t buf[512];
        size_t len = f.read(buf, sizeof(buf));
        br_sha256_update(&ctx, buf, len);
    }
    br_sha256_out(&ctx, hash);
    f.close();

    char actualHash[65];
    for (int i = 0; i < 32; i++) {
        sprintf(&actualHash[i * 2], "%02x", hash[i]);
    }
    actualHash[64] = 0;

    DEBUG.printf("Calculated checksum: %s\n", actualHash);
    DEBUG.printf("Expected checksum:   %s\n", expectedChecksum.c_str());

    return expectedChecksum.equalsIgnoreCase(actualHash);
}

bool downloadAndUpdate() {

  sendUpdateStatus("downloading", firmware_id, "none");
  delay(100);

  String firmwareURL = String(baseURL) + "/api/firmware/" + firmware_id + "/download";
  
  httpsClient.setInsecure();
  HTTPClient https;

  

  if (!https.begin(httpsClient, firmwareURL)) {
    DEBUG.println("❌ HTTPS begin failed");
    sendUpdateStatus("failed", firmware_id, "HTTPS begin failed");
    delay(100);
    return false;
  }

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    DEBUG.printf("❌ HTTP GET failed, code: %d\n", httpCode);
    sendUpdateStatus("failed", firmware_id, "HTTP GET failed: " + String(httpCode));
    delay(100);
    https.end();
    return false;
  }

  int totalLength = https.getSize();
  if (totalLength <= 0) {
    DEBUG.println("❌ Invalid firmware size");
    sendUpdateStatus("failed", firmware_id, "Invalid firmware size");
    delay(100);
    https.end();
    return false;
  }

   //   DEBUG.printf("Firmware size: %d bytes\n", totalLength);

  if (!Update.begin(totalLength)) {
    DEBUG.println("❌ Not enough space for OTA");
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
        DEBUG.println(msg);
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

  DEBUG.println("Expected checksum:   " + expectedChecksum);
  DEBUG.println("Calculated checksum: " + calculatedChecksum);

  if (expectedChecksum != calculatedChecksum) {
    DEBUG.println("❌ Checksum mismatch! Aborting update.");
    sendUpdateStatus("failed", firmware_id, "Checksum mismatch");
    delay(100);
    Update.end(); // do not commit
    return false;
  }

  if (!Update.end()) {
    DEBUG.printf("❌ OTA update error: %s\n", Update.getErrorString());
    sendUpdateStatus("failed", firmware_id, Update.getErrorString());
    delay(100);
    return false;
  }

  DEBUG.println("✅ OTA update successful. Rebooting...");
     sendUpdateStatus("success", firmware_id, "Update completed successfully");
  delay(100);
  https.end();
  
  ESP.restart();
  return true;
}




